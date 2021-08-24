#include "D3D12Device.h"

// https://devblogs.microsoft.com/directx/gettingstarted-dx12agility/
extern "C"
{
	_declspec(dllexport) extern const UINT D3D12SDKVersion = 4;
	_declspec(dllexport) extern const char* D3D12SDKPath   = ".\\D3D12\\";
}

AutoConsoleVariable<bool> CVar_DRED(
	"D3D12.DRED",
	"Enable Device Removed Extended Data\n"
	"DRED delivers automatic breadcrumbs as well as GPU page fault reporting\n",
	true);

using Microsoft::WRL::ComPtr;

void D3D12Device::ReportLiveObjects()
{
#ifdef _DEBUG
	ComPtr<IDXGIDebug> DXGIDebug;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(DXGIDebug.GetAddressOf()))))
	{
		DXGIDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_IGNORE_INTERNAL);
	}
#endif
}

D3D12Device::D3D12Device()
	: LinkedDevice(this)
	, Profiler(1)
{
}

D3D12Device::~D3D12Device()
{
	if (CVar_DRED && DeviceRemovedFence)
	{
		// Need to gracefully exit the event
		DeviceRemovedFence->Signal(UINT64_MAX);
		BOOL b = UnregisterWaitEx(DeviceRemovedWaitHandle, INVALID_HANDLE_VALUE);
		assert(b);
	}
}

void D3D12Device::Initialize(const DeviceOptions& Options)
{
#ifdef _DEBUG
	InitializeDXGIObjects(true);
#else
	InitializeDXGIObjects(false);
#endif

	// The D3D debug layer (as well as Microsoft PIX and other graphics debugger
	// tools using an injection library) is not compatible with Nsight Aftermath!
	// If Aftermath detects that any of these tools are present it will fail
	// initialization.
	DeviceOptions OverrideOptions = Options;
#ifdef NVIDIA_NSIGHT_AFTERMATH
	OverrideOptions.EnableDebugLayer		 = false;
	OverrideOptions.EnableGpuBasedValidation = false;
	AftermathCrashTracker.Initialize();
#endif

	// Enable the D3D12 debug layer
	if (OverrideOptions.EnableDebugLayer || OverrideOptions.EnableGpuBasedValidation)
	{
		// NOTE: Enabling the debug layer after creating the ID3D12Device will cause the DX runtime to remove the
		// device.
		ComPtr<ID3D12Debug> Debug;
		if (SUCCEEDED(::D3D12GetDebugInterface(IID_PPV_ARGS(Debug.ReleaseAndGetAddressOf()))))
		{
			if (OverrideOptions.EnableDebugLayer)
			{
				Debug->EnableDebugLayer();
			}
		}

		ComPtr<ID3D12Debug3> Debug3;
		if (SUCCEEDED(Debug.As(&Debug3)))
		{
			Debug3->SetEnableGPUBasedValidation(OverrideOptions.EnableGpuBasedValidation);
		}

		ComPtr<ID3D12Debug5> Debug5;
		if (SUCCEEDED(Debug.As(&Debug5)))
		{
			Debug5->SetEnableAutoName(OverrideOptions.EnableAutoDebugName);
		}
	}

	if (CVar_DRED)
	{
		ComPtr<ID3D12DeviceRemovedExtendedDataSettings> DREDSettings;
		VERIFY_D3D12_API(::D3D12GetDebugInterface(IID_PPV_ARGS(DREDSettings.GetAddressOf())));

		// Turn on auto-breadcrumbs and page fault reporting.
		DREDSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
		DREDSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
	}
}

void D3D12Device::InitializeDevice(const DeviceFeatures& Features)
{
	VERIFY_D3D12_API(
		::D3D12CreateDevice(Adapter4.Get(), Features.FeatureLevel, IID_PPV_ARGS(Device.ReleaseAndGetAddressOf())));

#ifdef NVIDIA_NSIGHT_AFTERMATH
	AftermathCrashTracker.RegisterDevice(D3D12Device.Get());
#endif

	Device.As(&Device5);
	Device.As(&InfoQueue1);

	D3D12_DESCRIPTOR_HEAP_TYPE Types[] = { D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
										   D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
										   D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
										   D3D12_DESCRIPTOR_HEAP_TYPE_DSV };
	for (size_t i = 0; i < std::size(Types); ++i)
	{
		DescriptorHandleIncrementSizeCache[i] = Device->GetDescriptorHandleIncrementSize(Types[i]);
	}

	ComPtr<ID3D12InfoQueue> InfoQueue;
	if (SUCCEEDED(Device.As(&InfoQueue)))
	{
		VERIFY_D3D12_API(InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE));
		VERIFY_D3D12_API(InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE));
		VERIFY_D3D12_API(InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE));

		// Suppress messages based on their severity level
		// D3D12_MESSAGE_SEVERITY Severities[] = { D3D12_MESSAGE_SEVERITY_INFO };

		// Suppress individual messages by their ID
		// D3D12_MESSAGE_ID MessageIDs[] =
		//{
		//};

		// D3D12_INFO_QUEUE_FILTER InfoQueueFilter = {};
		// InfoQueueFilter.DenyList.NumSeverities	= ARRAYSIZE(Severities);
		// InfoQueueFilter.DenyList.pSeverityList	= Severities;
		// InfoQueueFilter.DenyList.NumIDs = ARRAYSIZE(MessageIDs);
		// InfoQueueFilter.DenyList.pIDList = MessageIDs;

		// ASSERTD3D12APISUCCEEDED(InfoQueue->PushStorageFilter(&InfoQueueFilter));
	}

	DeviceRemovedWaitHandle = INVALID_HANDLE_VALUE;
	if (CVar_DRED)
	{
		// DRED
		// https://docs.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device5-removedevice#remarks
		VERIFY_D3D12_API(
			Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(DeviceRemovedFence.ReleaseAndGetAddressOf())));

		DeviceRemovedEvent.create();
		// When a device is removed, it signals all fences to UINT64_MAX, we can use this to register events prior to
		// what happened.
		VERIFY_D3D12_API(DeviceRemovedFence->SetEventOnCompletion(UINT64_MAX, DeviceRemovedEvent.get()));

		RegisterWaitForSingleObject(
			&DeviceRemovedWaitHandle,
			DeviceRemovedEvent.get(),
			OnDeviceRemoved,
			Device.Get(),
			INFINITE,
			0);
	}

	if (Features.WaveOperation)
	{
		D3D12Feature<D3D12_FEATURE_D3D12_OPTIONS1> Options1 = {};
		if (SUCCEEDED(Device->CheckFeatureSupport(Options1.Feature, &Options1.FeatureSupportData, sizeof(Options1))))
		{
			if (!Options1->WaveOps)
			{
				throw std::runtime_error("Wave operation not supported on device");
			}
		}
	}

	if (Features.Raytracing)
	{
		D3D12Feature<D3D12_FEATURE_D3D12_OPTIONS5> Options5 = {};
		if (SUCCEEDED(Device->CheckFeatureSupport(Options5.Feature, &Options5.FeatureSupportData, sizeof(Options5))))
		{
			if (Options5->RaytracingTier < D3D12_RAYTRACING_TIER_1_0)
			{
				throw std::runtime_error("Raytracing not supported on device");
			}
		}
	}

	// https://microsoft.github.io/DirectX-Specs/d3d/HLSL_ShaderModel6_6.html
	// https://microsoft.github.io/DirectX-Specs/d3d/HLSL_ShaderModel6_6.html#dynamic-resource
	if (Features.DynamicResources)
	{
		D3D12Feature<D3D12_FEATURE_SHADER_MODEL> ShaderModel;
		ShaderModel.FeatureSupportData.HighestShaderModel = D3D_SHADER_MODEL_6_6;
		if (SUCCEEDED(
				Device->CheckFeatureSupport(ShaderModel.Feature, &ShaderModel.FeatureSupportData, sizeof(ShaderModel))))
		{
			if (ShaderModel->HighestShaderModel < D3D_SHADER_MODEL_6_6)
			{
				throw std::runtime_error("Dynamic resources not supported on device");
			}
		}

		D3D12Feature<D3D12_FEATURE_D3D12_OPTIONS> Options;
		if (SUCCEEDED(Device->CheckFeatureSupport(Options.Feature, &Options.FeatureSupportData, sizeof(Options))))
		{
			if (Options->ResourceBindingTier < D3D12_RESOURCE_BINDING_TIER_3)
			{
				throw std::runtime_error("Dynamic resources not supported on device");
			}
		}
	}

	LinkedDevice.Initialize();

	Profiler.Initialize(Device.Get(), LinkedDevice.GetGraphicsQueue()->GetFrequency());
}

D3D12RootSignature D3D12Device::CreateRootSignature(
	std::function<void(RootSignatureBuilder&)> Configurator,
	bool									   AddDescriptorTableRootParameters)
{
	RootSignatureBuilder Builder = {};
	Configurator(Builder);
	if (AddDescriptorTableRootParameters)
	{
		AddDescriptorTableRootParameterToBuilder(Builder);
	}

	return D3D12RootSignature(GetD3D12Device(), Builder);
}

D3D12RaytracingPipelineState D3D12Device::CreateRaytracingPipelineState(
	std::function<void(RaytracingPipelineStateBuilder&)> Configurator)
{
	RaytracingPipelineStateBuilder Builder = {};
	Configurator(Builder);

	return D3D12RaytracingPipelineState(GetD3D12Device5(), Builder);
}

void D3D12Device::OnDeviceRemoved(PVOID Context, BOOLEAN)
{
	ID3D12Device* D3D12Device	= static_cast<ID3D12Device*>(Context);
	HRESULT		  RemovedReason = D3D12Device->GetDeviceRemovedReason();
	if (FAILED(RemovedReason))
	{
		ComPtr<ID3D12DeviceRemovedExtendedData> DRED;
		VERIFY_D3D12_API(D3D12Device->QueryInterface(IID_PPV_ARGS(DRED.GetAddressOf())));
		D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT AutoBreadcrumbsOutput = {};
		D3D12_DRED_PAGE_FAULT_OUTPUT	   PageFaultOutput		 = {};
		VERIFY_D3D12_API(DRED->GetAutoBreadcrumbsOutput(&AutoBreadcrumbsOutput));
		VERIFY_D3D12_API(DRED->GetPageFaultAllocationOutput(&PageFaultOutput));

		// TODO: Log breadcrumbs and page fault, right now is not logged because im too lazy to implement it, i just
		// used Watch window to see breadcrumbs and page fault lol..
	}
}

void D3D12Device::InitializeDXGIObjects(bool Debug)
{
	UINT Flags = Debug ? DXGI_CREATE_FACTORY_DEBUG : 0;
	// Create DXGIFactory
	VERIFY_D3D12_API(::CreateDXGIFactory2(Flags, IID_PPV_ARGS(Factory6.ReleaseAndGetAddressOf())));

	// Enumerate hardware for an adapter that supports D3D12
	ComPtr<IDXGIAdapter4> pAdapter4;
	UINT				  AdapterID = 0;
	while (SUCCEEDED(Factory6->EnumAdapterByGpuPreference(
		AdapterID,
		DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
		IID_PPV_ARGS(pAdapter4.ReleaseAndGetAddressOf()))))
	{
		if (SUCCEEDED(::D3D12CreateDevice(pAdapter4.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr)))
		{
			Adapter4 = pAdapter4;
			break;
		}

		AdapterID++;
	}
}

void D3D12Device::AddDescriptorTableRootParameterToBuilder(RootSignatureBuilder& RootSignatureBuilder)
{
	// TODO: Maybe consider this as a fallback options when SM6.6 dynamic resource binding is integrated
	/* Descriptor Tables */

	// ShaderResource
	D3D12DescriptorTable SRVTable;
	{
		// constexpr D3D12_DESCRIPTOR_RANGE_FLAGS Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE |
		// D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
		constexpr D3D12_DESCRIPTOR_RANGE_FLAGS Flags =
			D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;

		SRVTable.AddSRVRange<0, 100>(UINT_MAX, Flags, 0); // g_Texture2DTable
		SRVTable.AddSRVRange<0, 101>(UINT_MAX, Flags, 0); // g_Texture2DUINT4Table
		SRVTable.AddSRVRange<0, 102>(UINT_MAX, Flags, 0); // g_Texture2DArrayTable
		SRVTable.AddSRVRange<0, 103>(UINT_MAX, Flags, 0); // g_TextureCubeTable
		SRVTable.AddSRVRange<0, 104>(UINT_MAX, Flags, 0); // g_ByteAddressBufferTable
	}
	RootSignatureBuilder.AddDescriptorTable(SRVTable);

	// UnorderedAccess
	D3D12DescriptorTable UAVTable;
	{
		// constexpr D3D12_DESCRIPTOR_RANGE_FLAGS Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE |
		// D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
		constexpr D3D12_DESCRIPTOR_RANGE_FLAGS Flags =
			D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;

		UAVTable.AddUAVRange<0, 100>(UINT_MAX, Flags, 0); // g_RWTexture2DTable
		UAVTable.AddUAVRange<0, 101>(UINT_MAX, Flags, 0); // g_RWTexture2DArrayTable
		UAVTable.AddUAVRange<0, 102>(UINT_MAX, Flags, 0); // g_RWByteAddressBufferTable
	}
	RootSignatureBuilder.AddDescriptorTable(UAVTable);

	// Sampler
	D3D12DescriptorTable SamplerTable;
	{
		constexpr D3D12_DESCRIPTOR_RANGE_FLAGS Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

		SamplerTable.AddSamplerRange<0, 100>(UINT_MAX, Flags, 0); // g_SamplerTable
	}
	RootSignatureBuilder.AddDescriptorTable(SamplerTable);
}

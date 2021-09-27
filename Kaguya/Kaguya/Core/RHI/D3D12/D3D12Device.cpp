#include "D3D12Device.h"

using Microsoft::WRL::ComPtr;

// https://devblogs.microsoft.com/directx/gettingstarted-dx12agility/
extern "C"
{
	_declspec(dllexport) extern const UINT D3D12SDKVersion = 4;
	_declspec(dllexport) extern const char* D3D12SDKPath   = ".\\D3D12\\";
}

static ConsoleVariable CVar_Dred(
	"D3D12.DRED",
	"Enable Device Removed Extended Data\n"
	"DRED delivers automatic breadcrumbs as well as GPU page fault reporting\n",
	true);

const char* GetD3D12MessageSeverity(D3D12_MESSAGE_SEVERITY Severity)
{
	// clang-format off
	switch (Severity)
	{
	case D3D12_MESSAGE_SEVERITY_CORRUPTION: return "[Corruption]";
	case D3D12_MESSAGE_SEVERITY_ERROR:		return "[Error]";
	case D3D12_MESSAGE_SEVERITY_WARNING:	return "[Warning]";
	case D3D12_MESSAGE_SEVERITY_INFO:		return "[Info]";
	case D3D12_MESSAGE_SEVERITY_MESSAGE:	return "[Message]";
	default:								return "<unknown>";
	}
	// clang-format on
}

void D3D12Device::ReportLiveObjects()
{
#ifdef _DEBUG
	ComPtr<IDXGIDebug> Debug;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(Debug.GetAddressOf()))))
	{
		Debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_IGNORE_INTERNAL);
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
	if (InfoQueue1)
	{
		InfoQueue1->UnregisterMessageCallback(std::exchange(CallbackCookie, 0));
	}

	if (CVar_Dred && DeviceRemovedFence)
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
	InitializeDxgiObjects(true);
#else
	InitializeDxgiObjects(false);
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
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(Debug.ReleaseAndGetAddressOf()))))
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

	if (CVar_Dred)
	{
		ComPtr<ID3D12DeviceRemovedExtendedDataSettings> DredSettings;
		VERIFY_D3D12_API(D3D12GetDebugInterface(IID_PPV_ARGS(DredSettings.GetAddressOf())));

		// Turn on auto-breadcrumbs and page fault reporting.
		DredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
		DredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
	}
}

void D3D12Device::InitializeDevice(const DeviceFeatures& Features)
{
	VERIFY_D3D12_API(
		D3D12CreateDevice(Adapter3.Get(), Features.FeatureLevel, IID_PPV_ARGS(Device.ReleaseAndGetAddressOf())));

#ifdef NVIDIA_NSIGHT_AFTERMATH
	AftermathCrashTracker.RegisterDevice(Device.Get());
#endif

	VERIFY_D3D12_API(Device.As(&Device5));
	Device.As(&InfoQueue1);

	if (FAILED(FeatureSupport.Init(Device.Get())))
	{
		LOG_WARN("Failed to initialize CD3DX12FeatureSupport, certain features might be unavailable.");
	}

	constexpr D3D12_DESCRIPTOR_HEAP_TYPE Types[] = { D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
													 D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
													 D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
													 D3D12_DESCRIPTOR_HEAP_TYPE_DSV };
	for (size_t i = 0; i < std::size(Types); ++i)
	{
		DescriptorSizeCache[i] = Device->GetDescriptorHandleIncrementSize(Types[i]);
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
		// D3D12_MESSAGE_ID IDs[] = {};

		// D3D12_INFO_QUEUE_FILTER InfoQueueFilter = {};
		// InfoQueueFilter.DenyList.NumSeverities	= ARRAYSIZE(Severities);
		// InfoQueueFilter.DenyList.pSeverityList	= Severities;
		// InfoQueueFilter.DenyList.NumIDs			= ARRAYSIZE(IDs);
		// InfoQueueFilter.DenyList.pIDList		= IDs;
		// VERIFY_D3D12_API(InfoQueue->PushStorageFilter(&InfoQueueFilter));
	}

	if (InfoQueue1)
	{
		VERIFY_D3D12_API(InfoQueue1->RegisterMessageCallback(
			[](D3D12_MESSAGE_CATEGORY Category,
			   D3D12_MESSAGE_SEVERITY Severity,
			   D3D12_MESSAGE_ID		  ID,
			   LPCSTR				  pDescription,
			   void*				  pContext)
			{
				LOG_ERROR("Severity: {}\n{}", GetD3D12MessageSeverity(Severity), pDescription);
			},
			D3D12_MESSAGE_CALLBACK_FLAG_NONE,
			nullptr,
			&CallbackCookie));
	}

	DeviceRemovedWaitHandle = INVALID_HANDLE_VALUE;
	if (CVar_Dred)
	{
		// Dred
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
		if (!FeatureSupport.WaveOps())
		{
			throw std::runtime_error("Wave operation not supported on device");
		}
	}

	if (Features.Raytracing)
	{
		if (FeatureSupport.RaytracingTier() < D3D12_RAYTRACING_TIER_1_0)
		{
			throw std::runtime_error("Raytracing not supported on device");
		}
	}

	// https://microsoft.github.io/DirectX-Specs/d3d/HLSL_ShaderModel6_6.html
	// https://microsoft.github.io/DirectX-Specs/d3d/HLSL_ShaderModel6_6.html#dynamic-resource
	if (Features.DynamicResources)
	{
		if (FeatureSupport.HighestShaderModel() < D3D_SHADER_MODEL_6_6 ||
			FeatureSupport.ResourceBindingTier() < D3D12_RESOURCE_BINDING_TIER_3)
		{
			throw std::runtime_error("Dynamic resources not supported on device");
		}
	}

	LinkedDevice.Initialize();

	Profiler.Initialize(Device.Get(), LinkedDevice.GetGraphicsQueue()->GetFrequency());
}

D3D12RootSignature D3D12Device::CreateRootSignature(Delegate<void(RootSignatureBuilder&)> Configurator)
{
	RootSignatureBuilder Builder = {};
	Configurator(Builder);
	if (!Builder.IsLocal())
	{
		// If a root signature is local we don't add bindless descriptor table because it will conflict with global root
		// signature
		AddDescriptorTableRootParameterToBuilder(Builder);
	}

	return D3D12RootSignature(this, Builder);
}

D3D12RaytracingPipelineState D3D12Device::CreateRaytracingPipelineState(
	Delegate<void(RaytracingPipelineStateBuilder&)> Configurator)
{
	RaytracingPipelineStateBuilder Builder = {};
	Configurator(Builder);

	return D3D12RaytracingPipelineState(GetD3D12Device5(), Builder);
}

void D3D12Device::OnDeviceRemoved(PVOID Context, BOOLEAN)
{
	auto	D3D12Device	  = static_cast<ID3D12Device*>(Context);
	HRESULT RemovedReason = D3D12Device->GetDeviceRemovedReason();
	if (FAILED(RemovedReason))
	{
		ComPtr<ID3D12DeviceRemovedExtendedData> Dred;
		if (SUCCEEDED(D3D12Device->QueryInterface(IID_PPV_ARGS(Dred.GetAddressOf()))))
		{
			D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT AutoBreadcrumbsOutput = {};
			D3D12_DRED_PAGE_FAULT_OUTPUT	   PageFaultOutput		 = {};

			if (SUCCEEDED(Dred->GetAutoBreadcrumbsOutput(&AutoBreadcrumbsOutput)))
			{
				for (const D3D12_AUTO_BREADCRUMB_NODE* Node = AutoBreadcrumbsOutput.pHeadAutoBreadcrumbNode; Node;
					 Node									= Node->pNext)
				{
					INT32 LastCompletedOp = *Node->pLastBreadcrumbValue;

					LOG_WARN(
						LR"({} Commandlist "{1}" on CommandQueue "{2}", {0:d}; completed of {4})",
						L"[DRED]",
						Node->pCommandListDebugNameW ? Node->pCommandListDebugNameW : L"<unknown>",
						Node->pCommandQueueDebugNameW ? Node->pCommandQueueDebugNameW : L"<unknown>",
						LastCompletedOp,
						Node->BreadcrumbCount);

					INT32 FirstOp = std::max(LastCompletedOp - 5, 0);
					INT32 LastOp  = std::min(LastCompletedOp + 5, INT32(Node->BreadcrumbCount) - 1);

					for (INT32 Op = FirstOp; Op <= LastOp; ++Op)
					{
						D3D12_AUTO_BREADCRUMB_OP BreadcrumbOp = Node->pCommandHistory[Op];
						LOG_WARN(
							LR"(    Op: {0:d};, {1} {2})",
							Op,
							GetAutoBreadcrumbOpString(BreadcrumbOp),
							Op + 1 == LastCompletedOp ? TEXT("- Last completed") : TEXT(""));
					}
				}
			}

			if (SUCCEEDED(Dred->GetPageFaultAllocationOutput(&PageFaultOutput)))
			{
				LOG_WARN("[DRED] PageFault at VA GPUAddress: {0:x};", PageFaultOutput.PageFaultVA);

				LOG_WARN("[DRED] Active objects with VA ranges that match the faulting VA:");
				for (const D3D12_DRED_ALLOCATION_NODE* Node = PageFaultOutput.pHeadExistingAllocationNode; Node;
					 Node									= Node->pNext)
				{
					LOG_WARN(
						L"    Name: {} (Type: {})",
						Node->ObjectNameW ? Node->ObjectNameW : L"<unknown>",
						GetDredAllocationTypeString(Node->AllocationType));
				}

				LOG_WARN("[DRED] Recent freed objects with VA ranges that match the faulting VA:");
				for (const D3D12_DRED_ALLOCATION_NODE* Node = PageFaultOutput.pHeadRecentFreedAllocationNode; Node;
					 Node									= Node->pNext)
				{
					LOG_WARN(
						L"    Name: {} (Type: {})",
						Node->ObjectNameW ? Node->ObjectNameW : L"<unknown>",
						GetDredAllocationTypeString(Node->AllocationType));
				}
			}
		}
	}
}

void D3D12Device::InitializeDxgiObjects(bool Debug)
{
	UINT FactoryFlags = Debug ? DXGI_CREATE_FACTORY_DEBUG : 0;
	// Create DXGIFactory
	VERIFY_D3D12_API(CreateDXGIFactory2(FactoryFlags, IID_PPV_ARGS(Factory6.ReleaseAndGetAddressOf())));

	// Enumerate hardware for an adapter that supports D3D12
	ComPtr<IDXGIAdapter3> AdapterIterator;
	UINT				  AdapterId = 0;
	while (SUCCEEDED(Factory6->EnumAdapterByGpuPreference(
		AdapterId,
		DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
		IID_PPV_ARGS(AdapterIterator.ReleaseAndGetAddressOf()))))
	{
		if (SUCCEEDED(
				D3D12CreateDevice(AdapterIterator.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr)))
		{
			Adapter3 = std::move(AdapterIterator);
			break;
		}

		++AdapterId;
	}
}

void D3D12Device::AddDescriptorTableRootParameterToBuilder(RootSignatureBuilder& RootSignatureBuilder)
{
	// TODO: Maybe consider this as a fallback options when SM6.6 dynamic resource binding is integrated
	/* Descriptor Tables */

	// ShaderResource
	D3D12DescriptorTable ShaderResourceTable(3);
	{
		constexpr D3D12_DESCRIPTOR_RANGE_FLAGS Flags =
			D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;

		ShaderResourceTable.AddSRVRange<0, 100>(UINT_MAX, Flags, 0); // g_Texture2DTable
		ShaderResourceTable.AddSRVRange<0, 101>(UINT_MAX, Flags, 0); // g_Texture2DArrayTable
		ShaderResourceTable.AddSRVRange<0, 102>(UINT_MAX, Flags, 0); // g_TextureCubeTable
	}
	RootSignatureBuilder.AddDescriptorTable(ShaderResourceTable);

	// UnorderedAccess
	D3D12DescriptorTable UnorderedAccessTable(2);
	{
		constexpr D3D12_DESCRIPTOR_RANGE_FLAGS Flags =
			D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;

		UnorderedAccessTable.AddUAVRange<0, 100>(UINT_MAX, Flags, 0); // g_RWTexture2DTable
		UnorderedAccessTable.AddUAVRange<0, 101>(UINT_MAX, Flags, 0); // g_RWTexture2DArrayTable
	}
	RootSignatureBuilder.AddDescriptorTable(UnorderedAccessTable);

	// Sampler
	D3D12DescriptorTable SamplerTable(1);
	{
		constexpr D3D12_DESCRIPTOR_RANGE_FLAGS Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

		SamplerTable.AddSamplerRange<0, 100>(UINT_MAX, Flags, 0); // g_SamplerTable
	}
	RootSignatureBuilder.AddDescriptorTable(SamplerTable);

	// g_SamplerPointWrap
	RootSignatureBuilder.AddStaticSampler<0, 101>(D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_WRAP, 16);
	// g_SamplerPointClamp
	RootSignatureBuilder.AddStaticSampler<1, 101>(D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 16);
	// g_SamplerLinearWrap
	RootSignatureBuilder.AddStaticSampler<2, 101>(D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, 16);
	// g_SamplerLinearClamp
	RootSignatureBuilder.AddStaticSampler<3, 101>(
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		16);
	// g_SamplerAnisotropicWrap
	RootSignatureBuilder.AddStaticSampler<4, 101>(D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_WRAP, 16);
	// g_SamplerAnisotropicClamp
	RootSignatureBuilder.AddStaticSampler<5, 101>(D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 16);
}

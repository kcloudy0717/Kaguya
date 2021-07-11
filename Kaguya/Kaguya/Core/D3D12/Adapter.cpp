#include "Adapter.h"
#include "Device.h"

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

void Adapter::ReportLiveObjects()
{
#ifdef _DEBUG
	ComPtr<IDXGIDebug> DXGIDebug;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&DXGIDebug))))
	{
		DXGIDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_IGNORE_INTERNAL);
	}
#endif
}

Adapter::Adapter()
	: Device(this)
{
}

Adapter::~Adapter()
{
	if (CVar_DRED && DeviceRemovedFence)
	{
		// Need to gracefully exit the event
		DeviceRemovedFence->Signal(UINT64_MAX);
		BOOL b = UnregisterWaitEx(DeviceRemovedWaitHandle, INVALID_HANDLE_VALUE);
		assert(b);
	}
}

void Adapter::Initialize(const DeviceOptions& Options)
{
#ifdef _DEBUG
	InitializeDXGIObjects(true);
#else
	InitializeDXGIObjects(false);
#endif

	// Enable the D3D12 debug layer
	if (Options.EnableDebugLayer || Options.EnableGpuBasedValidation)
	{
		// NOTE: Enabling the debug layer after creating the ID3D12Device will cause the DX runtime to remove the
		// device.
		ComPtr<ID3D12Debug> Debug;
		if (SUCCEEDED(::D3D12GetDebugInterface(IID_PPV_ARGS(Debug.ReleaseAndGetAddressOf()))))
		{
			if (Options.EnableDebugLayer)
			{
				Debug->EnableDebugLayer();
			}
		}

		ComPtr<ID3D12Debug3> Debug3;
		if (SUCCEEDED(Debug.As(&Debug3)))
		{
			Debug3->SetEnableGPUBasedValidation(Options.EnableGpuBasedValidation);
		}

		ComPtr<ID3D12Debug5> Debug5;
		if (SUCCEEDED(Debug.As(&Debug5)))
		{
			Debug5->SetEnableAutoName(Options.EnableAutoDebugName);
		}
	}

	if (CVar_DRED)
	{
		ComPtr<ID3D12DeviceRemovedExtendedDataSettings> DREDSettings;
		ASSERTD3D12APISUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&DREDSettings)));

		// Turn on auto-breadcrumbs and page fault reporting.
		DREDSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
		DREDSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
	}
}

void Adapter::InitializeDevice(const DeviceFeatures& Features)
{
	ASSERTD3D12APISUCCEEDED(
		::D3D12CreateDevice(Adapter4.Get(), Features.FeatureLevel, IID_PPV_ARGS(D3D12Device.ReleaseAndGetAddressOf())));

	D3D12Device.As(&D3D12Device5);

	ComPtr<ID3D12InfoQueue> InfoQueue;
	if (SUCCEEDED(D3D12Device.As(&InfoQueue)))
	{
		InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

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
		HRESULT hr = D3D12Device->CreateFence(
			0,
			D3D12_FENCE_FLAG_NONE,
			IID_PPV_ARGS(DeviceRemovedFence.ReleaseAndGetAddressOf()));

		DeviceRemovedEvent.create();
		// When a device is removed, it signals all fences to UINT64_MAX, we can use this to register events prior to
		// what happened.
		hr = DeviceRemovedFence->SetEventOnCompletion(UINT64_MAX, DeviceRemovedEvent.get());

		RegisterWaitForSingleObject(
			&DeviceRemovedWaitHandle,
			DeviceRemovedEvent.get(),
			OnDeviceRemoved,
			D3D12Device.Get(),
			INFINITE,
			0);
	}

	if (Features.WaveOperation)
	{
		D3D12Feature<D3D12_FEATURE_D3D12_OPTIONS1> Options1 = {};
		if (SUCCEEDED(
				D3D12Device->CheckFeatureSupport(Options1.Feature, &Options1.FeatureSupportData, sizeof(Options1))))
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
		if (SUCCEEDED(
				D3D12Device->CheckFeatureSupport(Options5.Feature, &Options5.FeatureSupportData, sizeof(Options5))))
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
		if (SUCCEEDED(D3D12Device->CheckFeatureSupport(
				ShaderModel.Feature,
				&ShaderModel.FeatureSupportData,
				sizeof(ShaderModel))))
		{
			if (ShaderModel->HighestShaderModel < D3D_SHADER_MODEL_6_6)
			{
				throw std::runtime_error("Dynamic resources not supported on device");
			}
		}

		D3D12Feature<D3D12_FEATURE_D3D12_OPTIONS> Options;
		if (SUCCEEDED(D3D12Device->CheckFeatureSupport(Options.Feature, &Options.FeatureSupportData, sizeof(Options))))
		{
			if (Options->ResourceBindingTier < D3D12_RESOURCE_BINDING_TIER_3)
			{
				throw std::runtime_error("Dynamic resources not supported on device");
			}
		}
	}

	Device.Initialize();
}

IDXGIAdapter4* Adapter::GetAdapter4() const
{
	return Adapter4.Get();
}

ID3D12Device* Adapter::GetD3D12Device() const
{
	return D3D12Device.Get();
}

ID3D12Device5* Adapter::GetD3D12Device5() const
{
	return D3D12Device5.Get();
}

void Adapter::OnDeviceRemoved(PVOID Context, BOOLEAN)
{
	ID3D12Device* D3D12Device	= static_cast<ID3D12Device*>(Context);
	HRESULT		  RemovedReason = D3D12Device->GetDeviceRemovedReason();
	if (FAILED(RemovedReason))
	{
		ComPtr<ID3D12DeviceRemovedExtendedData> Dred;
		ASSERTD3D12APISUCCEEDED(D3D12Device->QueryInterface(IID_PPV_ARGS(&Dred)));
		D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT DredAutoBreadcrumbsOutput = {};
		D3D12_DRED_PAGE_FAULT_OUTPUT	   DredPageFaultOutput		 = {};
		ASSERTD3D12APISUCCEEDED(Dred->GetAutoBreadcrumbsOutput(&DredAutoBreadcrumbsOutput));
		ASSERTD3D12APISUCCEEDED(Dred->GetPageFaultAllocationOutput(&DredPageFaultOutput));

		// TODO: Log breadcrumbs and page fault
		// Haven't experienced TDR yet, so when I do, fill this out
	}
}

void Adapter::InitializeDXGIObjects(bool Debug)
{
	UINT Flags = Debug ? DXGI_CREATE_FACTORY_DEBUG : 0;
	// Create DXGIFactory
	ASSERTD3D12APISUCCEEDED(::CreateDXGIFactory2(Flags, IID_PPV_ARGS(Factory6.ReleaseAndGetAddressOf())));

	// Enumerate hardware for an adapter that supports D3D12
	ComPtr<IDXGIAdapter4> pAdapter4;
	UINT				  AdapterID = 0;
	while (Factory6->EnumAdapterByGpuPreference(
			   AdapterID,
			   DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
			   IID_PPV_ARGS(pAdapter4.ReleaseAndGetAddressOf())) != DXGI_ERROR_NOT_FOUND)
	{
		if (SUCCEEDED(pAdapter4->GetDesc3(&AdapterDesc)))
		{
			if (AdapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				// Skip SOFTWARE adapters
				continue;
			}
		}

		if (SUCCEEDED(::D3D12CreateDevice(pAdapter4.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)))
		{
			Adapter4 = pAdapter4;
			break;
		}

		AdapterID++;
	}
}

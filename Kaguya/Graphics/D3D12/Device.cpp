#include "pch.h"
#include "Device.h"
#include <Core/Console.h>

using Microsoft::WRL::ComPtr;

AutoConsoleVariable<bool> CVar_DRED(
	"D3D12.DRED",
	"Enable Device Removed Extended Data\n"
	"DRED delivers automatic breadcrumbs as well as GPU page fault reporting\n",
	true);

static AutoConsoleVariable<int> CVar_GlobalResourceViewHeapSize(
	"D3D12.GlobalResourceViewHeapSize",
	"Global Resource View Heap Size",
	4096);

static AutoConsoleVariable<int> CVar_GlobalSamplerHeapSize(
	"D3D12.GlobalSamplerHeapSize",
	"Global Sampler Heap Size",
	D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE);

// https://devblogs.microsoft.com/directx/gettingstarted-dx12agility/
extern "C"
{
	_declspec(dllexport) extern const UINT D3D12SDKVersion = 4;
	_declspec(dllexport) extern const char* D3D12SDKPath   = ".\\D3D12\\";
}

void Device::ReportLiveObjects()
{
#ifdef _DEBUG
	ComPtr<IDXGIDebug> DXGIDebug;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&DXGIDebug))))
	{
		DXGIDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_IGNORE_INTERNAL);
	}
#endif
}

Device::Device(_In_ IDXGIAdapter4* pAdapter, const DeviceOptions& Options)
	: GraphicsQueue(this, D3D12_COMMAND_LIST_TYPE_DIRECT)
	, AsyncComputeQueue(this, D3D12_COMMAND_LIST_TYPE_COMPUTE)
	, CopyQueue1(this, D3D12_COMMAND_LIST_TYPE_COPY)
	, CopyQueue2(this, D3D12_COMMAND_LIST_TYPE_COPY)
	, ResourceDescriptorHeap(this)
	, SamplerDescriptorHeap(this)
	, RenderTargetDescriptorHeap(this)
	, DepthStencilDescriptorHeap(this)
{
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
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&DREDSettings)));

		// Turn on auto-breadcrumbs and page fault reporting.
		DREDSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
		DREDSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
	}

	ThrowIfFailed(::D3D12CreateDevice(pAdapter, Options.FeatureLevel, IID_PPV_ARGS(pDevice.ReleaseAndGetAddressOf())));

	pDevice.As(&pDevice5);

	ComPtr<ID3D12InfoQueue> InfoQueue;
	if (SUCCEEDED(pDevice.As(&InfoQueue)))
	{
		InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, Options.BreakOnCorruption);
		InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, Options.BreakOnError);
		InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, Options.BreakOnWarning);
	}

	DeviceRemovedWaitHandle = INVALID_HANDLE_VALUE;
	if (CVar_DRED)
	{
		// DRED
		// https://docs.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device5-removedevice#remarks
		HRESULT hr =
			pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(DeviceRemovedFence.ReleaseAndGetAddressOf()));

		DeviceRemovedEvent.create();
		// When a device is removed, it signals all fences to UINT64_MAX, we can use this to register events prior to
		// what happened.
		hr = DeviceRemovedFence->SetEventOnCompletion(UINT64_MAX, DeviceRemovedEvent.get());

		RegisterWaitForSingleObject(
			&DeviceRemovedWaitHandle,
			DeviceRemovedEvent.get(),
			OnDeviceRemoved,
			pDevice.Get(),
			INFINITE,
			0);
	}
}

Device::~Device()
{
	if (CVar_DRED)
	{
		// Need to gracefully exit the event
		DeviceRemovedFence->Signal(UINT64_MAX);
		BOOL b = UnregisterWaitEx(DeviceRemovedWaitHandle, INVALID_HANDLE_VALUE);
		assert(b);
	}
}

void Device::Initialize(const DeviceFeatures& Features)
{
	if (Features.WaveOperation)
	{
		D3D12Feature<D3D12_FEATURE_D3D12_OPTIONS1> Options1 = {};
		if (SUCCEEDED(pDevice->CheckFeatureSupport(Options1.Feature, &Options1.FeatureSupportData, sizeof(Options1))))
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
		if (SUCCEEDED(pDevice->CheckFeatureSupport(Options5.Feature, &Options5.FeatureSupportData, sizeof(Options5))))
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
		if (SUCCEEDED(pDevice->CheckFeatureSupport(
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
		if (SUCCEEDED(pDevice->CheckFeatureSupport(Options.Feature, &Options.FeatureSupportData, sizeof(Options))))
		{
			if (Options->ResourceBindingTier < D3D12_RESOURCE_BINDING_TIER_3)
			{
				throw std::runtime_error("Dynamic resources not supported on device");
			}
		}
	}

	GraphicsQueue.Initialize();
	AsyncComputeQueue.Initialize();
	CopyQueue1.Initialize();
	CopyQueue2.Initialize();

#if _DEBUG
	GraphicsQueue.GetCommandQueue()->SetName(L"3D");
	AsyncComputeQueue.GetCommandQueue()->SetName(L"Async Compute");
	CopyQueue1.GetCommandQueue()->SetName(L"Copy 1");
	CopyQueue2.GetCommandQueue()->SetName(L"Copy 2");
#endif

	ResourceDescriptorHeap.Create(CVar_GlobalResourceViewHeapSize, true, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	SamplerDescriptorHeap.Create(CVar_GlobalSamplerHeapSize, true, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	RenderTargetDescriptorHeap.Create(512, false, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	DepthStencilDescriptorHeap.Create(512, false, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

#if _DEBUG
	ResourceDescriptorHeap.SetName(L"Resource Descriptor Heap");
	SamplerDescriptorHeap.SetName(L"Sampler Descriptor Heap");

	RenderTargetDescriptorHeap.SetName(L"Render Target Descriptor Heap");
	DepthStencilDescriptorHeap.SetName(L"Depth Stencil Descriptor Heap");
#endif

	// TODO: Implement a task-graph for rendering work
	const unsigned int NumThreads = 1;
	AvailableCommandContexts.reserve(NumThreads);
	for (unsigned int i = 0; i < NumThreads; ++i)
	{
		AvailableCommandContexts.push_back(
			std::make_unique<CommandContext>(this, ECommandQueueType::Direct, D3D12_COMMAND_LIST_TYPE_DIRECT));
	}
	AvailableAsyncCommandContexts.reserve(NumThreads);
	for (unsigned int i = 0; i < NumThreads; ++i)
	{
		AvailableAsyncCommandContexts.push_back(
			std::make_unique<CommandContext>(this, ECommandQueueType::AsyncCompute, D3D12_COMMAND_LIST_TYPE_COMPUTE));
	}

	CopyContext1 = std::make_unique<CommandContext>(this, ECommandQueueType::Copy1, D3D12_COMMAND_LIST_TYPE_COPY);
	CopyContext2 = std::make_unique<CommandContext>(this, ECommandQueueType::Copy2, D3D12_COMMAND_LIST_TYPE_COPY);
}

ID3D12Device* Device::GetDevice() const
{
	return pDevice.Get();
}

ID3D12Device5* Device::GetDevice5() const
{
	return pDevice5.Get();
}

CommandQueue* Device::GetCommandQueue(ECommandQueueType Type)
{
	switch (Type)
	{
	case ECommandQueueType::Direct:
		return &GraphicsQueue;
	case ECommandQueueType::AsyncCompute:
		return &AsyncComputeQueue;
	case ECommandQueueType::Copy1:
		return &CopyQueue1;
	case ECommandQueueType::Copy2:
		return &CopyQueue2;
	default:
		assert(false && "Should not get here");
		return nullptr;
	}
}

D3D12_RESOURCE_ALLOCATION_INFO Device::GetResourceAllocationInfo(const D3D12_RESOURCE_DESC& ResourceDesc)
{
	UINT64 Hash = CityHash64((const char*)&ResourceDesc, sizeof(D3D12_RESOURCE_DESC));

	{
		ScopedReadLock _(ResourceAllocationInfoTableLock);
		if (auto iter = ResourceAllocationInfoTable.find(Hash); iter != ResourceAllocationInfoTable.end())
		{
			return iter->second;
		}
	}

	ScopedWriteLock _(ResourceAllocationInfoTableLock);

	D3D12_RESOURCE_ALLOCATION_INFO ResourceAllocationInfo = GetDevice()->GetResourceAllocationInfo(0, 1, &ResourceDesc);
	ResourceAllocationInfoTable.insert(std::make_pair(Hash, ResourceAllocationInfo));

	return ResourceAllocationInfo;
}

bool Device::ResourceSupport4KBAlignment(D3D12_RESOURCE_DESC& ResourceDesc)
{
	// Refer to MSDN and samples
	// 4KB alignment is only available for read only textures
	if (!(ResourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET ||
		  ResourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL ||
		  ResourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) &&
		ResourceDesc.SampleDesc.Count == 1)
	{
		// Since we are using small resources we can take advantage of 4KB
		// resource alignments. As long as the most detailed mip can fit in an
		// allocation less than 64KB, 4KB alignments can be used.
		//
		// When dealing with MSAA textures the rules are similar, but the minimum
		// alignment is 64KB for a texture whose most detailed mip can fit in an
		// allocation less than 4MB.
		ResourceDesc.Alignment = D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT;

		D3D12_RESOURCE_ALLOCATION_INFO ResourceAllocationInfo = GetResourceAllocationInfo(ResourceDesc);
		if (ResourceAllocationInfo.Alignment != D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT)
		{
			// If the alignment requested is not granted, then let D3D tell us
			// the alignment that needs to be used for these resources.
			ResourceDesc.Alignment = 0;
			return false;
		}

		return true;
	}

	return false;
}

void Device::OnDeviceRemoved(PVOID Context, BOOLEAN)
{
	ID3D12Device* D3D12Device	= static_cast<ID3D12Device*>(Context);
	HRESULT		  RemovedReason = D3D12Device->GetDeviceRemovedReason();
	if (FAILED(RemovedReason))
	{
		ComPtr<ID3D12DeviceRemovedExtendedData> Dred;
		ThrowIfFailed(D3D12Device->QueryInterface(IID_PPV_ARGS(&Dred)));
		D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT DredAutoBreadcrumbsOutput = {};
		D3D12_DRED_PAGE_FAULT_OUTPUT	   DredPageFaultOutput		 = {};
		ThrowIfFailed(Dred->GetAutoBreadcrumbsOutput(&DredAutoBreadcrumbsOutput));
		ThrowIfFailed(Dred->GetPageFaultAllocationOutput(&DredPageFaultOutput));

		// TODO: Log breadcrumbs and page fault
		// Haven't experienced TDR yet, so when I do, fill this out
	}
}

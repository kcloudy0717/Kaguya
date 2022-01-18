#include "D3D12Device.h"

// https://devblogs.microsoft.com/directx/gettingstarted-dx12agility/
extern "C"
{
	_declspec(dllexport) extern const UINT D3D12SDKVersion = 700;
	_declspec(dllexport) extern const char* D3D12SDKPath   = ".\\D3D12\\";
}

static ConsoleVariable CVar_Dred(
	"D3D12.DRED",
	"Enable device removed extended data\n"
	"DRED delivers automatic breadcrumbs as well as GPU page fault reporting\n",
	true);

static ConsoleVariable CVar_AsyncPsoCompilation(
	"D3D12.AsyncPsoCompile",
	"Enables asynchronous pipeline state object compilation",
	false);

D3D12Device::D3D12Device(const DeviceOptions& Options)
	: Device(InitializeDevice(Options))
	, Device1(DeviceQueryInterface<ID3D12Device1>())
	, Device5(DeviceQueryInterface<ID3D12Device5>())
	, FeatureSupport(InitializeFeatureSupport(Options))
	, DescriptorSizeCache(InitializeDescriptorSizeCache())
	, Dred(Device.Get())
	, LinkedDevice(this)
	, Profiler(1, Device.Get(), LinkedDevice.GetGraphicsQueue()->GetFrequency())
	, PsoCompilationThreadPool(std::make_unique<ThreadPool>())
	, Library(!Options.CachePath.empty() ? std::make_unique<D3D12PipelineLibrary>(this, Options.CachePath) : nullptr)
{
	ARC<ID3D12InfoQueue> InfoQueue;
	if (SUCCEEDED(Device->QueryInterface(IID_PPV_ARGS(&InfoQueue))))
	{
		VERIFY_D3D12_API(InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE));
		VERIFY_D3D12_API(InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE));
		VERIFY_D3D12_API(InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE));

		// Suppress messages based on their severity level
		D3D12_MESSAGE_SEVERITY Severities[] = { D3D12_MESSAGE_SEVERITY_INFO };

		// Suppress individual messages by their ID
		D3D12_MESSAGE_ID IDs[] = {
			D3D12_MESSAGE_ID_LOADPIPELINE_NAMENOTFOUND, // Could happen when we are trying to load a PSO but is not
														// present, then we will use this warning to create the PSO
			D3D12_MESSAGE_ID_LOADPIPELINE_INVALIDDESC,	// Could happen when shader is modified and the resulting
														// serialized PSO is not valid
			D3D12_MESSAGE_ID_STOREPIPELINE_DUPLICATENAME,
			D3D12_MESSAGE_ID_CREATEPIPELINELIBRARY_DRIVERVERSIONMISMATCH,
			D3D12_MESSAGE_ID_CREATERESOURCE_STATE_IGNORED
		};

		D3D12_INFO_QUEUE_FILTER InfoQueueFilter = {};
		InfoQueueFilter.DenyList.NumSeverities	= ARRAYSIZE(Severities);
		InfoQueueFilter.DenyList.pSeverityList	= Severities;
		InfoQueueFilter.DenyList.NumIDs			= ARRAYSIZE(IDs);
		InfoQueueFilter.DenyList.pIDList		= IDs;
		VERIFY_D3D12_API(InfoQueue->PushStorageFilter(&InfoQueueFilter));
	}
}

void D3D12Device::CreateDxgiFactory(bool Debug)
{
	UINT FactoryFlags = Debug ? DXGI_CREATE_FACTORY_DEBUG : 0;
	// Create DXGIFactory
	VERIFY_D3D12_API(CreateDXGIFactory2(FactoryFlags, IID_PPV_ARGS(&Factory6)));
}

bool D3D12Device::AllowAsyncPsoCompilation() const noexcept
{
	return CVar_AsyncPsoCompilation;
}

void D3D12Device::OnBeginFrame()
{
	Profiler.OnBeginFrame();
}

void D3D12Device::OnEndFrame()
{
	Profiler.OnEndFrame();
}

void D3D12Device::BeginCapture(const std::filesystem::path& Path) const
{
	PIXCaptureParameters CaptureParameters			= {};
	CaptureParameters.GpuCaptureParameters.FileName = Path.c_str();
	CaptureStatus									= PIXBeginCapture(PIX_CAPTURE_GPU, &CaptureParameters);
}

void D3D12Device::EndCapture() const
{
	if (SUCCEEDED(CaptureStatus))
	{
		PIXEndCapture(FALSE);
	}
}

void D3D12Device::WaitIdle()
{
	LinkedDevice.WaitIdle();
}

std::unique_ptr<D3D12RootSignature> D3D12Device::CreateRootSignature(RootSignatureDesc& Desc)
{
	if (!Desc.IsLocal())
	{
		// If a root signature is local we don't add bindless descriptor table because it will conflict with global root
		// signature
		AddBindlessParameterToDesc(Desc);
	}

	return std::make_unique<D3D12RootSignature>(this, Desc);
}

std::unique_ptr<D3D12RaytracingPipelineState> D3D12Device::CreateRaytracingPipelineState(RaytracingPipelineStateDesc& Desc)
{
	return std::make_unique<D3D12RaytracingPipelineState>(this, Desc);
}

void D3D12Device::ReportLiveObjects()
{
#ifdef _DEBUG
	ARC<IDXGIDebug> Debug;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&Debug))))
	{
		Debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_IGNORE_INTERNAL);
	}
#endif
}

void D3D12Device::OnDeviceRemoved(PVOID Context, BOOLEAN)
{
	auto	D3D12Device	  = static_cast<ID3D12Device*>(Context);
	HRESULT RemovedReason = D3D12Device->GetDeviceRemovedReason();
	if (FAILED(RemovedReason))
	{
		ARC<ID3D12DeviceRemovedExtendedData> Dred;
		if (SUCCEEDED(D3D12Device->QueryInterface(IID_PPV_ARGS(&Dred))))
		{
			D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT AutoBreadcrumbsOutput = {};
			D3D12_DRED_PAGE_FAULT_OUTPUT	   PageFaultOutput		 = {};

			if (SUCCEEDED(Dred->GetAutoBreadcrumbsOutput(&AutoBreadcrumbsOutput)))
			{
				for (const D3D12_AUTO_BREADCRUMB_NODE* Node = AutoBreadcrumbsOutput.pHeadAutoBreadcrumbNode; Node;
					 Node									= Node->pNext)
				{
					INT32 LastCompletedOp = static_cast<INT32>(*Node->pLastBreadcrumbValue);

					LUNA_LOG(
						D3D12RHI,
						Error,
						LR"({0} Commandlist "{1}" on CommandQueue "{2}", {3} completed of {4})",
						L"[DRED]",
						Node->pCommandListDebugNameW ? Node->pCommandListDebugNameW : L"<unknown>",
						Node->pCommandQueueDebugNameW ? Node->pCommandQueueDebugNameW : L"<unknown>",
						LastCompletedOp,
						Node->BreadcrumbCount);

					INT32 FirstOp = std::max(LastCompletedOp - 5, 0);
					INT32 LastOp  = std::min(LastCompletedOp + 5, std::max(INT32(Node->BreadcrumbCount) - 1, 0));

					for (INT32 Op = FirstOp; Op <= LastOp; ++Op)
					{
						D3D12_AUTO_BREADCRUMB_OP BreadcrumbOp = Node->pCommandHistory[Op];
						LUNA_LOG(
							D3D12RHI,
							Error,
							LR"(    Op: {0:d}, {1} {2})",
							Op,
							GetAutoBreadcrumbOpString(BreadcrumbOp),
							Op + 1 == LastCompletedOp ? TEXT("- Last completed") : TEXT(""));
					}
				}
			}

			if (SUCCEEDED(Dred->GetPageFaultAllocationOutput(&PageFaultOutput)))
			{
				LUNA_LOG(D3D12RHI, Error, "[DRED] PageFault at VA GPUAddress: {0:x};", PageFaultOutput.PageFaultVA);

				LUNA_LOG(D3D12RHI, Error, "[DRED] Active objects with VA ranges that match the faulting VA:");
				for (const D3D12_DRED_ALLOCATION_NODE* Node = PageFaultOutput.pHeadExistingAllocationNode; Node;
					 Node									= Node->pNext)
				{
					LUNA_LOG(
						D3D12RHI,
						Error,
						L"    Name: {} (Type: {})",
						Node->ObjectNameW ? Node->ObjectNameW : L"<unknown>",
						GetDredAllocationTypeString(Node->AllocationType));
				}

				LUNA_LOG(D3D12RHI, Error, "[DRED] Recent freed objects with VA ranges that match the faulting VA:");
				for (const D3D12_DRED_ALLOCATION_NODE* Node = PageFaultOutput.pHeadRecentFreedAllocationNode; Node;
					 Node									= Node->pNext)
				{
					LUNA_LOG(
						D3D12RHI,
						Error,
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
	CreateDxgiFactory(Debug);

	// Enumerate hardware for an adapter that supports D3D12
	ARC<IDXGIAdapter3> AdapterIterator;
	UINT			   AdapterId = 0;
	while (SUCCEEDED(Factory6->EnumAdapterByGpuPreference(
		AdapterId,
		DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
		IID_PPV_ARGS(AdapterIterator.ReleaseAndGetAddressOf()))))
	{
		if (SUCCEEDED(D3D12CreateDevice(
				AdapterIterator.Get(),
				D3D_FEATURE_LEVEL_12_0,
				__uuidof(ID3D12Device),
				nullptr)))
		{
			Adapter3 = std::move(AdapterIterator);
			if (SUCCEEDED(Adapter3->GetDesc2(&AdapterDesc)))
			{
				LUNA_LOG(D3D12RHI, Info, "Adapter Vendor: {}", GetRHIVendorString(static_cast<RHI_VENDOR>(AdapterDesc.VendorId)));
				LUNA_LOG(D3D12RHI, Info, L"Adapter: {}", AdapterDesc.Description);
				LUNA_LOG(D3D12RHI, Info, L"\tDedicated Video Memory: {}", AdapterDesc.DedicatedVideoMemory);
			}
			break;
		}

		++AdapterId;
	}
}

ARC<ID3D12Device> D3D12Device::InitializeDevice(const DeviceOptions& Options)
{
#ifdef _DEBUG
	InitializeDxgiObjects(true);
#else
	InitializeDxgiObjects(false);
#endif

	// Enable the D3D12 debug layer
	if (Options.EnableDebugLayer || Options.EnableGpuBasedValidation)
	{
		// NOTE: Enabling the debug layer after creating the ID3D12Device will cause the DX runtime to remove the
		// device.
		ARC<ID3D12Debug> Debug;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&Debug))))
		{
			if (Options.EnableDebugLayer)
			{
				Debug->EnableDebugLayer();
			}
		}

		ARC<ID3D12Debug3> Debug3;
		if (SUCCEEDED(Debug->QueryInterface(IID_PPV_ARGS(&Debug3))))
		{
			Debug3->SetEnableGPUBasedValidation(Options.EnableGpuBasedValidation);
		}

		ARC<ID3D12Debug5> Debug5;
		if (SUCCEEDED(Debug->QueryInterface(IID_PPV_ARGS(&Debug5))))
		{
			Debug5->SetEnableAutoName(Options.EnableAutoDebugName);
		}
	}

	if (CVar_Dred)
	{
		ARC<ID3D12DeviceRemovedExtendedDataSettings> DredSettings;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&DredSettings))))
		{
			LUNA_LOG(D3D12RHI, Info, "DRED Enabled");
			// Turn on auto-breadcrumbs and page fault reporting.
			DredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
			DredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
		}
	}
	else
	{
		LUNA_LOG(D3D12RHI, Info, "DRED Disabled");
	}

	ARC<ID3D12Device> Device;
	VERIFY_D3D12_API(D3D12CreateDevice(Adapter3.Get(), Options.FeatureLevel, IID_PPV_ARGS(&Device)));
	return Device;
}

CD3DX12FeatureSupport D3D12Device::InitializeFeatureSupport(const DeviceOptions& Options)
{
	CD3DX12FeatureSupport FeatureSupport;
	if (FAILED(FeatureSupport.Init(Device.Get())))
	{
		LUNA_LOG(D3D12RHI, Warn, "Failed to initialize CD3DX12FeatureSupport, certain features might be unavailable.");
	}

	if (Options.WaveOperation)
	{
		if (!FeatureSupport.WaveOps())
		{
			LUNA_LOG(D3D12RHI, Error, "Wave operation not supported on device");
		}
	}

	if (Options.Raytracing)
	{
		if (FeatureSupport.RaytracingTier() < D3D12_RAYTRACING_TIER_1_0)
		{
			LUNA_LOG(D3D12RHI, Error, "Raytracing not supported on device");
		}
	}

	// https://microsoft.github.io/DirectX-Specs/d3d/HLSL_ShaderModel6_6.html
	// https://microsoft.github.io/DirectX-Specs/d3d/HLSL_ShaderModel6_6.html#dynamic-resource
	if (Options.DynamicResources)
	{
		if (FeatureSupport.HighestShaderModel() < D3D_SHADER_MODEL_6_6 ||
			FeatureSupport.ResourceBindingTier() < D3D12_RESOURCE_BINDING_TIER_3)
		{
			LUNA_LOG(D3D12RHI, Error, "Dynamic resources not supported on device");
		}
	}

	if (Options.MeshShaders)
	{
		if (FeatureSupport.MeshShaderTier() < D3D12_MESH_SHADER_TIER_1)
		{
			LUNA_LOG(D3D12RHI, Error, "Mesh shaders not supported on device");
		}
	}

	BOOL EnhancedBarriersSupported = FeatureSupport.EnhancedBarriersSupported();
	if (!EnhancedBarriersSupported)
	{
		LUNA_LOG(D3D12RHI, Warn, "Enhanced barriers not supported");
	}

	return FeatureSupport;
}

D3D12Device::TDescriptorSizeCache D3D12Device::InitializeDescriptorSizeCache()
{
	TDescriptorSizeCache				 DescriptorSizeCache;
	constexpr D3D12_DESCRIPTOR_HEAP_TYPE Types[] = { D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
													 D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
													 D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
													 D3D12_DESCRIPTOR_HEAP_TYPE_DSV };
	for (size_t i = 0; i < std::size(Types); ++i)
	{
		DescriptorSizeCache[i] = Device->GetDescriptorHandleIncrementSize(Types[i]);
	}
	return DescriptorSizeCache;
}

void D3D12Device::AddBindlessParameterToDesc(RootSignatureDesc& Desc)
{
	// TODO: Maybe consider this as a fall back options when SM6.6 dynamic resource binding is integrated
	/* Descriptor Tables */

	constexpr D3D12_DESCRIPTOR_RANGE_FLAGS DescriptorDataVolatile = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;
	constexpr D3D12_DESCRIPTOR_RANGE_FLAGS DescriptorVolatile	  = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
	Desc.AddDescriptorTable(
			// ShaderResource
			D3D12DescriptorTable(4)
				.AddSRVRange<0, 100>(UINT_MAX, DescriptorDataVolatile, 0)  // g_ByteAddressBufferTable
				.AddSRVRange<0, 101>(UINT_MAX, DescriptorDataVolatile, 0)  // g_Texture2DTable
				.AddSRVRange<0, 102>(UINT_MAX, DescriptorDataVolatile, 0)  // g_Texture2DArrayTable
				.AddSRVRange<0, 103>(UINT_MAX, DescriptorDataVolatile, 0)) // g_TextureCubeTable
		.AddDescriptorTable(
			// UnorderedAccess
			D3D12DescriptorTable(2)
				.AddUAVRange<0, 100>(UINT_MAX, DescriptorDataVolatile, 0)  // g_RWTexture2DTable
				.AddUAVRange<0, 101>(UINT_MAX, DescriptorDataVolatile, 0)) // g_RWTexture2DArrayTable
		.AddDescriptorTable(
			// Sampler
			D3D12DescriptorTable(1)
				.AddSamplerRange<0, 100>(UINT_MAX, DescriptorVolatile, 0)); // g_SamplerTable

	constexpr D3D12_FILTER				 PointFilter  = D3D12_FILTER_MIN_MAG_MIP_POINT;
	constexpr D3D12_FILTER				 LinearFilter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	constexpr D3D12_FILTER				 Anisotropic  = D3D12_FILTER_ANISOTROPIC;
	constexpr D3D12_TEXTURE_ADDRESS_MODE Wrap		  = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	constexpr D3D12_TEXTURE_ADDRESS_MODE Clamp		  = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	constexpr D3D12_TEXTURE_ADDRESS_MODE Border		  = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	Desc.AddSampler<0, 101>(PointFilter, Wrap, 16)	// g_SamplerPointWrap
		.AddSampler<1, 101>(PointFilter, Clamp, 16) // g_SamplerPointClamp

		.AddSampler<2, 101>(LinearFilter, Wrap, 16)																					 // g_SamplerLinearWrap
		.AddSampler<3, 101>(LinearFilter, Clamp, 16)																				 // g_SamplerLinearClamp
		.AddSampler<4, 101>(LinearFilter, Border, 16, D3D12_COMPARISON_FUNC_LESS_EQUAL, D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK) // g_SamplerLinearBorder

		.AddSampler<5, 101>(Anisotropic, Wrap, 16)	 // g_SamplerAnisotropicWrap
		.AddSampler<6, 101>(Anisotropic, Clamp, 16); // g_SamplerAnisotropicClamp
}

D3D12Device::Dred::Dred(ID3D12Device* Device)
	: DeviceRemovedWaitHandle(INVALID_HANDLE_VALUE)
{
	if (CVar_Dred)
	{
		// Dred
		// https://docs.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device5-removedevice#remarks
		VERIFY_D3D12_API(Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&DeviceRemovedFence)));

		DeviceRemovedEvent.create();
		// When a device is removed, it signals all fences to UINT64_MAX, we can use this to register events prior
		// to what happened.
		VERIFY_D3D12_API(DeviceRemovedFence->SetEventOnCompletion(UINT64_MAX, DeviceRemovedEvent.get()));

		RegisterWaitForSingleObject(
			&DeviceRemovedWaitHandle,
			DeviceRemovedEvent.get(),
			OnDeviceRemoved,
			Device,
			INFINITE,
			0);
	}
}

D3D12Device::Dred::~Dred()
{
	if (DeviceRemovedFence)
	{
		// Need to gracefully exit the event
		DeviceRemovedFence->Signal(UINT64_MAX);
		BOOL b = UnregisterWaitEx(DeviceRemovedWaitHandle, INVALID_HANDLE_VALUE);
		assert(b);
	}
}

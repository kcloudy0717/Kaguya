#include "D3D12Device.h"

static ConsoleVariable CVar_Dred(
	"D3D12.DRED",
	"Enable device removed extended data\n"
	"DRED delivers automatic breadcrumbs as well as GPU page fault reporting\n",
	true);

static ConsoleVariable CVar_AsyncPsoCompilation(
	"D3D12.AsyncPsoCompile",
	"Enables asynchronous pipeline state object compilation",
	true);

namespace RHI
{
	D3D12Device::D3D12Device(const DeviceOptions& Options)
		: Device(InitializeDevice(Options))
		, Device1(DeviceQueryInterface<ID3D12Device1>())
		, Device5(DeviceQueryInterface<ID3D12Device5>())
		, AllNodeMask(D3D12NodeMask::FromIndex(Device->GetNodeCount() - 1))
		, FeatureSupport(InitializeFeatureSupport(Options))
		, DescriptorSizeCache(InitializeDescriptorSizeCache())
		, Dred(Device.Get())
		, LinkedDevice(this, D3D12NodeMask::FromIndex(0))
	{
		VERIFY_D3D12_API(DStorageGetFactory(IID_PPV_ARGS(&DStorageFactory)));

		DSTORAGE_QUEUE_DESC QueueDesc = {
			.SourceType = DSTORAGE_REQUEST_SOURCE_FILE,
			.Capacity	= DSTORAGE_MAX_QUEUE_CAPACITY,
			.Priority	= DSTORAGE_PRIORITY_NORMAL,
			.Name		= "DStorageQueue: File",
			.Device		= Device.Get()
		};
		VERIFY_D3D12_API(DStorageFactory->CreateQueue(&QueueDesc, IID_PPV_ARGS(&DStorageQueues[DSTORAGE_REQUEST_SOURCE_FILE])));

		QueueDesc = {
			.SourceType = DSTORAGE_REQUEST_SOURCE_MEMORY,
			.Capacity	= DSTORAGE_MAX_QUEUE_CAPACITY,
			.Priority	= DSTORAGE_PRIORITY_NORMAL,
			.Name		= "DStorageQueue: Memory",
			.Device		= Device.Get()
		};
		VERIFY_D3D12_API(DStorageFactory->CreateQueue(&QueueDesc, IID_PPV_ARGS(&DStorageQueues[DSTORAGE_REQUEST_SOURCE_MEMORY])));

		DStorageFence = D3D12Fence(this, 0);

		Arc<ID3D12InfoQueue> InfoQueue;
		if (SUCCEEDED(Device->QueryInterface(IID_PPV_ARGS(&InfoQueue))))
		{
			VERIFY_D3D12_API(InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE));
			VERIFY_D3D12_API(InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE));
			VERIFY_D3D12_API(InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE));

			// Suppress messages based on their severity level
			D3D12_MESSAGE_SEVERITY Severities[] = { D3D12_MESSAGE_SEVERITY_INFO };

			// Suppress individual messages by their ID
			// D3D12_MESSAGE_ID IDs[] = {};

			D3D12_INFO_QUEUE_FILTER InfoQueueFilter = {
				.DenyList = {
					.NumSeverities = ARRAYSIZE(Severities),
					.pSeverityList = Severities,
					//.NumIDs		   = ARRAYSIZE(IDs),
					//.pIDList	   = IDs
				}
			};
			VERIFY_D3D12_API(InfoQueue->PushStorageFilter(&InfoQueueFilter));
		}
	}

	void D3D12Device::CreateDxgiFactory()
	{
#ifdef _DEBUG
		InternalCreateDxgiFactory(true);
#else
		InternalCreateDxgiFactory(false);
#endif
	}

	bool D3D12Device::AllowAsyncPsoCompilation() const noexcept
	{
		return CVar_AsyncPsoCompilation;
	}

	bool D3D12Device::SupportsWaveIntrinsics() const noexcept
	{
		if (SUCCEEDED(FeatureSupport.GetStatus()))
		{
			return FeatureSupport.WaveOps();
		}
		return false;
	}

	bool D3D12Device::SupportsRaytracing() const noexcept
	{
		if (SUCCEEDED(FeatureSupport.GetStatus()))
		{
			return FeatureSupport.RaytracingTier() >= D3D12_RAYTRACING_TIER_1_0;
		}
		return false;
	}

	bool D3D12Device::SupportsDynamicResources() const noexcept
	{
		if (SUCCEEDED(FeatureSupport.GetStatus()))
		{
			return FeatureSupport.HighestShaderModel() >= D3D_SHADER_MODEL_6_6 ||
				   FeatureSupport.ResourceBindingTier() >= D3D12_RESOURCE_BINDING_TIER_3;
		}
		return false;
	}

	bool D3D12Device::SupportsMeshShaders() const noexcept
	{
		if (SUCCEEDED(FeatureSupport.GetStatus()))
		{
			return FeatureSupport.MeshShaderTier() >= D3D12_MESH_SHADER_TIER_1;
		}
		return false;
	}

	void D3D12Device::OnBeginFrame()
	{
		LinkedDevice.OnBeginFrame();
	}

	void D3D12Device::OnEndFrame()
	{
		LinkedDevice.OnEndFrame();
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

	D3D12SyncHandle D3D12Device::DStorageSubmit(DSTORAGE_REQUEST_SOURCE_TYPE Type)
	{
		UINT64 FenceValue = DStorageFence.Signal(DStorageQueues[Type]);
		DStorageQueues[Type]->Submit();
		return D3D12SyncHandle{ &DStorageFence, FenceValue };
	}

	D3D12RootSignature D3D12Device::CreateRootSignature(RootSignatureDesc& Desc)
	{
		return D3D12RootSignature(this, Desc);
	}

	D3D12RaytracingPipelineState D3D12Device::CreateRaytracingPipelineState(RaytracingPipelineStateDesc& Desc)
	{
		return D3D12RaytracingPipelineState(this, Desc);
	}

	// ======================================== Private ========================================
	void D3D12Device::ReportLiveObjects()
	{
#ifdef _DEBUG
		Arc<IDXGIDebug> Debug;
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
			Arc<ID3D12DeviceRemovedExtendedData> Dred;
			if (SUCCEEDED(D3D12Device->QueryInterface(IID_PPV_ARGS(&Dred))))
			{
				D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT AutoBreadcrumbsOutput = {};
				D3D12_DRED_PAGE_FAULT_OUTPUT	   PageFaultOutput		 = {};

				if (SUCCEEDED(Dred->GetAutoBreadcrumbsOutput(&AutoBreadcrumbsOutput)))
				{
					for (const D3D12_AUTO_BREADCRUMB_NODE* Node = AutoBreadcrumbsOutput.pHeadAutoBreadcrumbNode; Node; Node = Node->pNext)
					{
						INT32 LastCompletedOp = static_cast<INT32>(*Node->pLastBreadcrumbValue);

						auto CommandListName  = Node->pCommandListDebugNameW ? Node->pCommandListDebugNameW : L"<unknown>";
						auto CommandQueueName = Node->pCommandQueueDebugNameW ? Node->pCommandQueueDebugNameW : L"<unknown>";
						KAGUYA_LOG(D3D12RHI, Error, LR"({0} Commandlist "{1}" on CommandQueue "{2}", {3} completed of {4})", L"[DRED]", CommandListName, CommandQueueName, LastCompletedOp, Node->BreadcrumbCount);

						INT32 FirstOp = std::max(LastCompletedOp - 5, 0);
						INT32 LastOp  = std::min(LastCompletedOp + 5, std::max(INT32(Node->BreadcrumbCount) - 1, 0));

						for (INT32 Op = FirstOp; Op <= LastOp; ++Op)
						{
							D3D12_AUTO_BREADCRUMB_OP BreadcrumbOp = Node->pCommandHistory[Op];
							KAGUYA_LOG(D3D12RHI, Error, LR"(    Op: {0:d}, {1} {2})", Op, GetAutoBreadcrumbOpString(BreadcrumbOp), Op + 1 == LastCompletedOp ? TEXT("- Last completed") : TEXT(""));
						}
					}
				}

				if (SUCCEEDED(Dred->GetPageFaultAllocationOutput(&PageFaultOutput)))
				{
					KAGUYA_LOG(D3D12RHI, Error, "[DRED] PageFault at VA GPUAddress: {0:x};", PageFaultOutput.PageFaultVA);

					KAGUYA_LOG(D3D12RHI, Error, "[DRED] Active objects with VA ranges that match the faulting VA:");
					for (const D3D12_DRED_ALLOCATION_NODE* Node = PageFaultOutput.pHeadExistingAllocationNode; Node; Node = Node->pNext)
					{
						auto ObjectName = Node->ObjectNameW ? Node->ObjectNameW : L"<unknown>";
						KAGUYA_LOG(D3D12RHI, Error, L"    Name: {} (Type: {})", ObjectName, GetDredAllocationTypeString(Node->AllocationType));
					}

					KAGUYA_LOG(D3D12RHI, Error, "[DRED] Recent freed objects with VA ranges that match the faulting VA:");
					for (const D3D12_DRED_ALLOCATION_NODE* Node = PageFaultOutput.pHeadRecentFreedAllocationNode; Node; Node = Node->pNext)
					{
						auto ObjectName = Node->ObjectNameW ? Node->ObjectNameW : L"<unknown>";
						KAGUYA_LOG(D3D12RHI, Error, L"    Name: {} (Type: {})", ObjectName, GetDredAllocationTypeString(Node->AllocationType));
					}
				}
			}
		}
	}

	void D3D12Device::InternalCreateDxgiFactory(bool Debug)
	{
		UINT FactoryFlags = Debug ? DXGI_CREATE_FACTORY_DEBUG : 0;
		// Create DXGIFactory
		VERIFY_D3D12_API(CreateDXGIFactory2(FactoryFlags, IID_PPV_ARGS(&Factory6)));
	}

	Arc<ID3D12Device> D3D12Device::InitializeDevice(const DeviceOptions& Options)
	{
		CreateDxgiFactory();

		// Enumerate hardware for an adapter that supports D3D12
		Arc<IDXGIAdapter3> AdapterIterator;
		UINT			   AdapterId = 0;
		while (SUCCEEDED(Factory6->EnumAdapterByGpuPreference(AdapterId, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(AdapterIterator.ReleaseAndGetAddressOf()))))
		{
			if (SUCCEEDED(D3D12CreateDevice(AdapterIterator.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr)))
			{
				Adapter3 = std::move(AdapterIterator);
				if (SUCCEEDED(Adapter3->GetDesc2(&AdapterDesc)))
				{
					KAGUYA_LOG(D3D12RHI, Info, "Adapter Vendor: {}", GetRHIVendorString(static_cast<RHI_VENDOR>(AdapterDesc.VendorId)));
					KAGUYA_LOG(D3D12RHI, Info, L"Adapter: {}", AdapterDesc.Description);
				}
				break;
			}

			++AdapterId;
		}

		// Enable the D3D12 debug layer
		if (Options.EnableDebugLayer || Options.EnableGpuBasedValidation)
		{
			// NOTE: Enabling the debug layer after creating the ID3D12Device will cause the DX runtime to remove the
			// device.
			Arc<ID3D12Debug> Debug;
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&Debug))))
			{
				if (Options.EnableDebugLayer)
				{
					Debug->EnableDebugLayer();
				}
			}

			Arc<ID3D12Debug3> Debug3;
			if (SUCCEEDED(Debug->QueryInterface(IID_PPV_ARGS(&Debug3))))
			{
				Debug3->SetEnableGPUBasedValidation(Options.EnableGpuBasedValidation);
			}

			Arc<ID3D12Debug5> Debug5;
			if (SUCCEEDED(Debug->QueryInterface(IID_PPV_ARGS(&Debug5))))
			{
				Debug5->SetEnableAutoName(Options.EnableAutoDebugName);
			}
		}

		if (CVar_Dred)
		{
			Arc<ID3D12DeviceRemovedExtendedDataSettings> DredSettings;
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&DredSettings))))
			{
				KAGUYA_LOG(D3D12RHI, Info, "DRED Enabled");
				// Turn on auto-breadcrumbs and page fault reporting.
				DredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
				DredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
			}
		}
		else
		{
			KAGUYA_LOG(D3D12RHI, Info, "DRED Disabled");
		}

		Arc<ID3D12Device> Device;
		VERIFY_D3D12_API(D3D12CreateDevice(Adapter3.Get(), Options.FeatureLevel, IID_PPV_ARGS(&Device)));
		return Device;
	}

	CD3DX12FeatureSupport D3D12Device::InitializeFeatureSupport(const DeviceOptions& Options)
	{
		CD3DX12FeatureSupport FeatureSupport;
		if (FAILED(FeatureSupport.Init(Device.Get())))
		{
			KAGUYA_LOG(D3D12RHI, Warn, "Failed to initialize CD3DX12FeatureSupport, certain features might be unavailable.");
		}

		if (Options.WaveIntrinsics)
		{
			if (!FeatureSupport.WaveOps())
			{
				KAGUYA_LOG(D3D12RHI, Error, "Wave operation not supported on device");
			}
		}

		if (Options.Raytracing)
		{
			if (FeatureSupport.RaytracingTier() < D3D12_RAYTRACING_TIER_1_0)
			{
				KAGUYA_LOG(D3D12RHI, Error, "Raytracing not supported on device");
			}
		}

		// https://microsoft.github.io/DirectX-Specs/d3d/HLSL_ShaderModel6_6.html
		// https://microsoft.github.io/DirectX-Specs/d3d/HLSL_ShaderModel6_6.html#dynamic-resource
		if (Options.DynamicResources)
		{
			if (FeatureSupport.HighestShaderModel() < D3D_SHADER_MODEL_6_6 ||
				FeatureSupport.ResourceBindingTier() < D3D12_RESOURCE_BINDING_TIER_3)
			{
				KAGUYA_LOG(D3D12RHI, Error, "Dynamic resources not supported on device");
			}
		}

		if (Options.MeshShaders)
		{
			if (FeatureSupport.MeshShaderTier() < D3D12_MESH_SHADER_TIER_1)
			{
				KAGUYA_LOG(D3D12RHI, Error, "Mesh shaders not supported on device");
			}
		}

		BOOL EnhancedBarriersSupported = FeatureSupport.EnhancedBarriersSupported();
		if (!EnhancedBarriersSupported)
		{
			KAGUYA_LOG(D3D12RHI, Warn, "Enhanced barriers not supported");
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

	LPCWSTR D3D12Device::GetAutoBreadcrumbOpString(D3D12_AUTO_BREADCRUMB_OP Op)
	{
#define STRINGIFY(x) \
	case x:          \
		return L#x

		switch (Op)
		{
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_SETMARKER);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_BEGINEVENT);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_ENDEVENT);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_DRAWINSTANCED);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_DRAWINDEXEDINSTANCED);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_EXECUTEINDIRECT);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_DISPATCH);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_COPYBUFFERREGION);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_COPYTEXTUREREGION);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_COPYRESOURCE);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_COPYTILES);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCE);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_CLEARRENDERTARGETVIEW);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_CLEARUNORDEREDACCESSVIEW);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_CLEARDEPTHSTENCILVIEW);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_RESOURCEBARRIER);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_EXECUTEBUNDLE);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_PRESENT);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_RESOLVEQUERYDATA);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_BEGINSUBMISSION);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_ENDSUBMISSION);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT64);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCEREGION);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_WRITEBUFFERIMMEDIATE);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME1);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_SETPROTECTEDRESOURCESESSION);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME2);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES1);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_BUILDRAYTRACINGACCELERATIONSTRUCTURE);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_EMITRAYTRACINGACCELERATIONSTRUCTUREPOSTBUILDINFO);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_COPYRAYTRACINGACCELERATIONSTRUCTURE);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_DISPATCHRAYS);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_INITIALIZEMETACOMMAND);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_EXECUTEMETACOMMAND);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_ESTIMATEMOTION);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_RESOLVEMOTIONVECTORHEAP);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_SETPIPELINESTATE1);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_INITIALIZEEXTENSIONCOMMAND);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_EXECUTEEXTENSIONCOMMAND);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_DISPATCHMESH);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_ENCODEFRAME);
			STRINGIFY(D3D12_AUTO_BREADCRUMB_OP_RESOLVEENCODEROUTPUTMETADATA);
		}
		return L"<unknown>";
#undef STRINGIFY
	}

	LPCWSTR D3D12Device::GetDredAllocationTypeString(D3D12_DRED_ALLOCATION_TYPE Type)
	{
#define STRINGIFY(x) \
	case x:          \
		return L#x

		switch (Type)
		{
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_COMMAND_QUEUE);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_COMMAND_ALLOCATOR);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_PIPELINE_STATE);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_COMMAND_LIST);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_FENCE);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_DESCRIPTOR_HEAP);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_HEAP);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_QUERY_HEAP);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_COMMAND_SIGNATURE);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_PIPELINE_LIBRARY);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_VIDEO_DECODER);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_VIDEO_PROCESSOR);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_RESOURCE);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_PASS);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_CRYPTOSESSION);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_CRYPTOSESSIONPOLICY);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_PROTECTEDRESOURCESESSION);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_VIDEO_DECODER_HEAP);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_COMMAND_POOL);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_COMMAND_RECORDER);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_STATE_OBJECT);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_METACOMMAND);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_SCHEDULINGGROUP);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_VIDEO_MOTION_ESTIMATOR);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_VIDEO_MOTION_VECTOR_HEAP);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_VIDEO_EXTENSION_COMMAND);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_VIDEO_ENCODER);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_VIDEO_ENCODER_HEAP);
			STRINGIFY(D3D12_DRED_ALLOCATION_TYPE_INVALID);
		}
		return L"<unknown>";
#undef STRINGIFY
	}

	D3D12Device::Dred::Dred(ID3D12Device* Device)
		: DeviceRemovedWaitHandle(INVALID_HANDLE_VALUE) // Don't need to call CloseHandle based on RegisterWaitForSingleObject doc
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

			RegisterWaitForSingleObject(&DeviceRemovedWaitHandle, DeviceRemovedEvent.get(), OnDeviceRemoved, Device, INFINITE, 0);
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
} // namespace RHI

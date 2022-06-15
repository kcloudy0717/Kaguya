#include "D3D12LinkedDevice.h"
#include "D3D12Device.h"

static ConsoleVariable CVar_DescriptorAllocatorPageSize(
	"D3D12.DescriptorAllocatorPageSize",
	"Descriptor Allocator Page Size",
	2048);

static ConsoleVariable CVar_GlobalResourceViewHeapSize(
	"D3D12.GlobalResourceViewHeapSize",
	"Global Resource View Heap Size",
	65536);

static ConsoleVariable CVar_GlobalSamplerHeapSize(
	"D3D12.GlobalSamplerHeapSize",
	"Global Sampler Heap Size",
	D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE);

namespace RHI
{
	D3D12LinkedDevice::D3D12LinkedDevice(
		D3D12Device*  Parent,
		D3D12NodeMask NodeMask)
		: D3D12DeviceChild(Parent)
		, NodeMask(NodeMask)
		, GraphicsQueue(this, RHID3D12CommandQueueType::Direct)
		, AsyncComputeQueue(this, RHID3D12CommandQueueType::AsyncCompute)
		, CopyQueue1(this, RHID3D12CommandQueueType::Copy)
		, CopyQueue2(this, RHID3D12CommandQueueType::Upload)
		, Profiler(this, 1)
		, RtvHeapManager(this, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, CVar_DescriptorAllocatorPageSize)
		, DsvHeapManager(this, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, CVar_DescriptorAllocatorPageSize)
		, CbvSrvUavHeapManager(this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, CVar_DescriptorAllocatorPageSize)
		, ResourceDescriptorHeap(this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, CVar_GlobalResourceViewHeapSize)
		, SamplerDescriptorHeap(this, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, CVar_GlobalSamplerHeapSize)
		, GraphicsContext(this, RHID3D12CommandQueueType::Direct, D3D12_COMMAND_LIST_TYPE_DIRECT)
		, ComputeContext(this, RHID3D12CommandQueueType::AsyncCompute, D3D12_COMMAND_LIST_TYPE_COMPUTE)
		, CopyContext(this, RHID3D12CommandQueueType::Copy, D3D12_COMMAND_LIST_TYPE_COPY)
		, UploadContext(this, RHID3D12CommandQueueType::Upload, D3D12_COMMAND_LIST_TYPE_COPY)
		, NullRaytracingAccelerationStructure(this, nullptr)
	{
#if _DEBUG
		ResourceDescriptorHeap.SetName(L"Resource Descriptor Heap");
		SamplerDescriptorHeap.SetName(L"Sampler Descriptor Heap");
#endif
	}

	D3D12LinkedDevice::~D3D12LinkedDevice()
	{
		if (UploadSyncHandle)
		{
			UploadSyncHandle.WaitForCompletion();
		}
	}

	ID3D12Device* D3D12LinkedDevice::GetDevice() const noexcept
	{
		return GetParentDevice()->GetD3D12Device();
	}

	ID3D12Device5* D3D12LinkedDevice::GetDevice5() const noexcept
	{
		return GetParentDevice()->GetD3D12Device5();
	}

	D3D12NodeMask D3D12LinkedDevice::GetNodeMask() const noexcept
	{
		return NodeMask;
	}

	D3D12CommandQueue* D3D12LinkedDevice::GetCommandQueue(RHID3D12CommandQueueType Type) noexcept
	{
		// clang-format off
		switch (Type)
		{
			using enum RHID3D12CommandQueueType;
		case Direct:		return &GraphicsQueue;
		case AsyncCompute:	return &AsyncComputeQueue;
		case Copy:			return &CopyQueue1;
		case Upload:			return &CopyQueue2;
		}
		// clang-format on
		return nullptr;
	}

	D3D12CommandQueue* D3D12LinkedDevice::GetGraphicsQueue() noexcept
	{
		return GetCommandQueue(RHID3D12CommandQueueType::Direct);
	}

	D3D12CommandQueue* D3D12LinkedDevice::GetAsyncComputeQueue() noexcept
	{
		return GetCommandQueue(RHID3D12CommandQueueType::AsyncCompute);
	}

	D3D12CommandQueue* D3D12LinkedDevice::GetCopyQueue1() noexcept
	{
		return GetCommandQueue(RHID3D12CommandQueueType::Copy);
	}

	D3D12Profiler* D3D12LinkedDevice::GetProfiler() noexcept
	{
		return &Profiler;
	}

	D3D12DescriptorHeap& D3D12LinkedDevice::GetResourceDescriptorHeap() noexcept
	{
		return ResourceDescriptorHeap;
	}

	D3D12DescriptorHeap& D3D12LinkedDevice::GetSamplerDescriptorHeap() noexcept
	{
		return SamplerDescriptorHeap;
	}

	D3D12CommandContext& D3D12LinkedDevice::GetGraphicsContext()
	{
		return GraphicsContext;
	}

	D3D12CommandContext& D3D12LinkedDevice::GetComputeContext()
	{
		return ComputeContext;
	}

	D3D12CommandContext& D3D12LinkedDevice::GetCopyContext()
	{
		return CopyContext;
	}

	D3D12ShaderResourceView* D3D12LinkedDevice::GetNullRaytracingAcceleraionStructure()
	{
		return &NullRaytracingAccelerationStructure;
	}

	void D3D12LinkedDevice::OnBeginFrame()
	{
		Profiler.OnBeginFrame();
	}

	void D3D12LinkedDevice::OnEndFrame()
	{
		Profiler.OnEndFrame();
	}

	D3D12_RESOURCE_ALLOCATION_INFO D3D12LinkedDevice::GetResourceAllocationInfo(const D3D12_RESOURCE_DESC& Desc) const
	{
		const u64 Hash = Hash::Hash64(&Desc, sizeof(Desc));
		{
			RwLockReadGuard Guard(ResourceAllocationInfoTable.Mutex);
			if (auto Iter = ResourceAllocationInfoTable.Table.find(Hash);
				Iter != ResourceAllocationInfoTable.Table.end())
			{
				return Iter->second;
			}
		}

		RwLockWriteGuard Guard(ResourceAllocationInfoTable.Mutex);

		const D3D12_RESOURCE_ALLOCATION_INFO ResourceAllocationInfo = GetDevice()->GetResourceAllocationInfo(NodeMask, 1, &Desc);
		ResourceAllocationInfoTable.Table.insert(std::make_pair(Hash, ResourceAllocationInfo));

		return ResourceAllocationInfo;
	}

	bool D3D12LinkedDevice::ResourceSupport4KiBAlignment(D3D12_RESOURCE_DESC* Desc) const
	{
		// 4KiB alignment is only available for read only textures
		if (!(Desc->Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET ||
			  Desc->Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL ||
			  Desc->Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) &&
			Desc->SampleDesc.Count == 1)
		{
			// Since we are using small resources we can take advantage of 4KiB
			// resource alignments. As long as the most detailed mip can fit in an
			// allocation less than 64KiB, 4KiB alignments can be used.
			//
			// When dealing with MSAA textures the rules are similar, but the minimum
			// alignment is 64KiB for a texture whose most detailed mip can fit in an
			// allocation less than 4MiB.
			Desc->Alignment = D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT;

			D3D12_RESOURCE_ALLOCATION_INFO ResourceAllocationInfo = GetResourceAllocationInfo(*Desc);
			if (ResourceAllocationInfo.Alignment != D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT)
			{
				// If the alignment requested is not granted, then let D3D tell us
				// the alignment that needs to be used for these resources.
				Desc->Alignment = 0;
				return false;
			}

			return true;
		}

		return false;
	}

	void D3D12LinkedDevice::WaitIdle()
	{
		GraphicsQueue.WaitIdle();
		AsyncComputeQueue.WaitIdle();
		CopyQueue1.WaitIdle();
		CopyQueue2.WaitIdle();
	}

	void D3D12LinkedDevice::BeginResourceUpload()
	{
		if (UploadSyncHandle && UploadSyncHandle.IsComplete())
		{
			TrackedResources.clear();
		}

		UploadContext.Open();
	}

	D3D12SyncHandle D3D12LinkedDevice::EndResourceUpload(bool WaitForCompletion)
	{
		UploadContext->Close();
		UploadSyncHandle = UploadContext.Execute(WaitForCompletion);
		return UploadSyncHandle;
	}

	void D3D12LinkedDevice::Upload(const std::vector<D3D12_SUBRESOURCE_DATA>& Subresources, ID3D12Resource* Resource)
	{
		const auto					NumSubresources = static_cast<UINT>(Subresources.size());
		const UINT64				UploadSize		= GetRequiredIntermediateSize(Resource, 0, NumSubresources);
		const D3D12_HEAP_PROPERTIES HeapProperties	= CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD, NodeMask, NodeMask);
		const D3D12_RESOURCE_DESC	ResourceDesc	= CD3DX12_RESOURCE_DESC::Buffer(UploadSize);
		Arc<ID3D12Resource>			UploadResource;
		VERIFY_D3D12_API(GetDevice()->CreateCommittedResource(
			&HeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&ResourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(UploadResource.ReleaseAndGetAddressOf())));

		UpdateSubresources(
			UploadContext.GetGraphicsCommandList(),
			Resource,
			UploadResource.Get(),
			0,
			0,
			NumSubresources,
			Subresources.data());

		TrackedResources.push_back(std::move(UploadResource));
	}

	void D3D12LinkedDevice::Upload(const D3D12_SUBRESOURCE_DATA& Subresource, ID3D12Resource* Resource)
	{
		const UINT64				UploadSize	   = GetRequiredIntermediateSize(Resource, 0, 1);
		const D3D12_HEAP_PROPERTIES HeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD, NodeMask, NodeMask);
		const D3D12_RESOURCE_DESC	ResourceDesc   = CD3DX12_RESOURCE_DESC::Buffer(UploadSize);
		Arc<ID3D12Resource>			UploadResource;
		VERIFY_D3D12_API(GetDevice()->CreateCommittedResource(
			&HeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&ResourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(UploadResource.ReleaseAndGetAddressOf())));

		UpdateSubresources<1>(
			UploadContext.GetGraphicsCommandList(),
			Resource,
			UploadResource.Get(),
			0,
			0,
			1,
			&Subresource);

		TrackedResources.push_back(std::move(UploadResource));
	}

	void D3D12LinkedDevice::Upload(const void* Data, UINT64 SizeInBytes, ID3D12Resource* Resource)
	{
		const D3D12_SUBRESOURCE_DATA SubresourceData = {
			.pData		= Data,
			.RowPitch	= static_cast<LONG_PTR>(SizeInBytes),
			.SlicePitch = static_cast<LONG_PTR>(SizeInBytes),
		};
		Upload(SubresourceData, Resource);
	}
} // namespace RHI

#include "D3D12LinkedDevice.h"
#include "D3D12Device.h"

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
		, CopyQueue(this, RHID3D12CommandQueueType::Copy)
		, UploadQueue(this, RHID3D12CommandQueueType::Upload)
		, Profiler(this, 1)
		, RtvHeapManager(this, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 256)
		, DsvHeapManager(this, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 256)
		, SrvHeapManager(this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1024)
		, UavHeapManager(this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1024)
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

	D3D12CommandQueue* D3D12LinkedDevice::GetCommandQueue(RHID3D12CommandQueueType Type) noexcept
	{
		switch (Type)
		{
		case RHID3D12CommandQueueType::Direct: return &GraphicsQueue;
		case RHID3D12CommandQueueType::AsyncCompute: return &AsyncComputeQueue;
		case RHID3D12CommandQueueType::Copy: return &CopyQueue;
		case RHID3D12CommandQueueType::Upload: return &UploadQueue;
		}
		return nullptr;
	}

	void D3D12LinkedDevice::OnBeginFrame()
	{
		Profiler.OnBeginFrame();
	}

	void D3D12LinkedDevice::OnEndFrame()
	{
		Profiler.OnEndFrame();
		DeferredDeleteDescriptorQueue.Clean();
	}

	void D3D12LinkedDevice::WaitIdle()
	{
		GraphicsQueue.WaitIdle();
		AsyncComputeQueue.WaitIdle();
		CopyQueue.WaitIdle();
		UploadQueue.WaitIdle();
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

	void D3D12LinkedDevice::ReleaseDescriptor(DeferredDeleteDescriptor Descriptor)
	{
		D3D12SyncHandle GraphicsSyncHandle = GraphicsQueue.GetSyncHandle();
		D3D12SyncHandle ComputeSyncHandle  = AsyncComputeQueue.GetSyncHandle();
		// No pending workload, we can release immediately
		if (!GraphicsSyncHandle && !ComputeSyncHandle)
		{
			Descriptor.Release();
			return;
		}
		DeferredDeleteDescriptorQueue.Enqueue(GraphicsSyncHandle, ComputeSyncHandle, Descriptor);
	}
} // namespace RHI

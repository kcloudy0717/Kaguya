#include "D3D12LinkedDevice.h"
#include "D3D12Device.h"

using Microsoft::WRL::ComPtr;

static ConsoleVariable CVar_DescriptorAllocatorPageSize(
	"D3D12.DescriptorAllocatorPageSize",
	"Descriptor Allocator Page Size",
	4096);

static ConsoleVariable CVar_GlobalResourceViewHeapSize(
	"D3D12.GlobalResourceViewHeapSize",
	"Global Resource View Heap Size",
	65535);

static ConsoleVariable CVar_GlobalSamplerHeapSize(
	"D3D12.GlobalSamplerHeapSize",
	"Global Sampler Heap Size",
	D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE);

D3D12LinkedDevice::D3D12LinkedDevice(D3D12Device* Parent)
	: D3D12DeviceChild(Parent)
	, GraphicsQueue(this, RHID3D12CommandQueueType::Direct)
	, AsyncComputeQueue(this, RHID3D12CommandQueueType::AsyncCompute)
	, CopyQueue1(this, RHID3D12CommandQueueType::Copy1)
	, CopyQueue2(this, RHID3D12CommandQueueType::Copy2)
	, RtvAllocator(this, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, CVar_DescriptorAllocatorPageSize)
	, DsvAllocator(this, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, CVar_DescriptorAllocatorPageSize)
	, ResourceDescriptorHeap(this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, CVar_GlobalResourceViewHeapSize)
	, SamplerDescriptorHeap(this, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, CVar_GlobalSamplerHeapSize)
{
#if _DEBUG
	ResourceDescriptorHeap.SetName(L"Resource Descriptor Heap");
	SamplerDescriptorHeap.SetName(L"Sampler Descriptor Heap");
#endif
	using enum RHID3D12CommandQueueType;
	constexpr size_t NumThreads = 1;
	AvailableCommandContexts.reserve(NumThreads);
	for (unsigned int i = 0; i < NumThreads; ++i)
	{
		AvailableCommandContexts.push_back(std::make_unique<D3D12CommandContext>(this, Direct, D3D12_COMMAND_LIST_TYPE_DIRECT));
	}
	AvailableAsyncCommandContexts.reserve(NumThreads);
	for (unsigned int i = 0; i < NumThreads; ++i)
	{
		AvailableAsyncCommandContexts.push_back(std::make_unique<D3D12CommandContext>(this, AsyncCompute, D3D12_COMMAND_LIST_TYPE_COMPUTE));
	}
	CopyContext1 = std::make_unique<D3D12CommandContext>(this, Copy1, D3D12_COMMAND_LIST_TYPE_COPY);
	CopyContext2 = std::make_unique<D3D12CommandContext>(this, Copy2, D3D12_COMMAND_LIST_TYPE_COPY);
}

D3D12LinkedDevice::~D3D12LinkedDevice()
{
	if (UploadSyncHandle)
	{
		UploadSyncHandle.WaitForCompletion();
	}
}

ID3D12Device* D3D12LinkedDevice::GetDevice() const
{
	return GetParentDevice()->GetD3D12Device();
}

ID3D12Device5* D3D12LinkedDevice::GetDevice5() const
{
	return GetParentDevice()->GetD3D12Device5();
}

D3D12CommandQueue* D3D12LinkedDevice::GetCommandQueue(RHID3D12CommandQueueType Type)
{
	// clang-format off
	switch (Type)
	{
		using enum RHID3D12CommandQueueType;
	case Direct:		return &GraphicsQueue;
	case AsyncCompute:	return &AsyncComputeQueue;
	case Copy1:			return &CopyQueue1;
	case Copy2:			return &CopyQueue2;
	}
	// clang-format on
	return nullptr;
}

D3D12CommandQueue* D3D12LinkedDevice::GetGraphicsQueue()
{
	return GetCommandQueue(RHID3D12CommandQueueType::Direct);
}

D3D12CommandQueue* D3D12LinkedDevice::GetAsyncComputeQueue()
{
	return GetCommandQueue(RHID3D12CommandQueueType::AsyncCompute);
}

D3D12CommandQueue* D3D12LinkedDevice::GetCopyQueue1()
{
	return GetCommandQueue(RHID3D12CommandQueueType::Copy1);
}

D3D12DescriptorAllocator& D3D12LinkedDevice::GetRtvAllocator() noexcept
{
	return RtvAllocator;
}

D3D12DescriptorAllocator& D3D12LinkedDevice::GetDsvAllocator() noexcept
{
	return DsvAllocator;
}

D3D12DescriptorHeap& D3D12LinkedDevice::GetResourceDescriptorHeap() noexcept
{
	return ResourceDescriptorHeap;
}

D3D12DescriptorHeap& D3D12LinkedDevice::GetSamplerDescriptorHeap() noexcept
{
	return SamplerDescriptorHeap;
}

D3D12CommandContext& D3D12LinkedDevice::GetCommandContext(UINT ThreadIndex /*= 0*/)
{
	assert(ThreadIndex < AvailableCommandContexts.size());
	return *AvailableCommandContexts[ThreadIndex];
}

D3D12CommandContext& D3D12LinkedDevice::GetAsyncComputeCommandContext(UINT ThreadIndex /*= 0*/)
{
	assert(ThreadIndex < AvailableAsyncCommandContexts.size());
	return *AvailableAsyncCommandContexts[ThreadIndex];
}

D3D12CommandContext& D3D12LinkedDevice::GetCopyContext1()
{
	return *CopyContext1;
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

	CopyContext2->Open();
}

D3D12SyncHandle D3D12LinkedDevice::EndResourceUpload(bool WaitForCompletion)
{
	CopyContext2->Close();
	UploadSyncHandle = CopyContext2->Execute(WaitForCompletion);
	return UploadSyncHandle;
}

void D3D12LinkedDevice::Upload(const std::vector<D3D12_SUBRESOURCE_DATA>& Subresources, ID3D12Resource* Resource)
{
	auto				   NumSubresources = static_cast<UINT>(Subresources.size());
	UINT64				   UploadSize	   = GetRequiredIntermediateSize(Resource, 0, NumSubresources);
	D3D12_HEAP_PROPERTIES  HeapProperties  = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC	   ResourceDesc	   = CD3DX12_RESOURCE_DESC::Buffer(UploadSize);
	ComPtr<ID3D12Resource> UploadResource;
	VERIFY_D3D12_API(GetDevice()->CreateCommittedResource(
		&HeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&ResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(UploadResource.ReleaseAndGetAddressOf())));

	UpdateSubresources(
		CopyContext2->GetGraphicsCommandList(),
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
	UINT64				   UploadSize	  = GetRequiredIntermediateSize(Resource, 0, 1);
	D3D12_HEAP_PROPERTIES  HeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC	   ResourceDesc	  = CD3DX12_RESOURCE_DESC::Buffer(UploadSize);
	ComPtr<ID3D12Resource> UploadResource;
	VERIFY_D3D12_API(GetDevice()->CreateCommittedResource(
		&HeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&ResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(UploadResource.ReleaseAndGetAddressOf())));

	UpdateSubresources<1>(
		CopyContext2->GetGraphicsCommandList(),
		Resource,
		UploadResource.Get(),
		0,
		0,
		1,
		&Subresource);

	TrackedResources.push_back(std::move(UploadResource));
}

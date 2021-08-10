#include "D3D12LinkedDevice.h"
#include "D3D12Device.h"

using Microsoft::WRL::ComPtr;

static AutoConsoleVariable<int> CVar_GlobalResourceViewHeapSize(
	"D3D12.GlobalResourceViewHeapSize",
	"Global Resource View Heap Size",
	4096);

static AutoConsoleVariable<int> CVar_GlobalSamplerHeapSize(
	"D3D12.GlobalSamplerHeapSize",
	"Global Sampler Heap Size",
	D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE);

D3D12LinkedDevice::D3D12LinkedDevice(D3D12Device* Parent)
	: D3D12DeviceChild(Parent)
	, GraphicsQueue(this, D3D12_COMMAND_LIST_TYPE_DIRECT)
	, AsyncComputeQueue(this, D3D12_COMMAND_LIST_TYPE_COMPUTE)
	, CopyQueue1(this, D3D12_COMMAND_LIST_TYPE_COPY)
	, CopyQueue2(this, D3D12_COMMAND_LIST_TYPE_COPY)
	, ResourceDescriptorHeap(this)
	, SamplerDescriptorHeap(this)
	, RenderTargetDescriptorHeap(this)
	, DepthStencilDescriptorHeap(this)
{
}

D3D12LinkedDevice::~D3D12LinkedDevice()
{
}

void D3D12LinkedDevice::Initialize()
{
	GraphicsQueue.Initialize(ED3D12CommandQueueType::Direct);
	AsyncComputeQueue.Initialize(ED3D12CommandQueueType::AsyncCompute);
	CopyQueue1.Initialize(ED3D12CommandQueueType::Copy1);
	CopyQueue2.Initialize(ED3D12CommandQueueType::Copy2);

	ResourceDescriptorHeap.Initialize(CVar_GlobalResourceViewHeapSize, true, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	SamplerDescriptorHeap.Initialize(CVar_GlobalSamplerHeapSize, true, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	RenderTargetDescriptorHeap.Initialize(512, false, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	DepthStencilDescriptorHeap.Initialize(512, false, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

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
		AvailableCommandContexts.push_back(std::make_unique<D3D12CommandContext>(
			this,
			ED3D12CommandQueueType::Direct,
			D3D12_COMMAND_LIST_TYPE_DIRECT));
	}
	AvailableAsyncCommandContexts.reserve(NumThreads);
	for (unsigned int i = 0; i < NumThreads; ++i)
	{
		AvailableAsyncCommandContexts.push_back(std::make_unique<D3D12CommandContext>(
			this,
			ED3D12CommandQueueType::AsyncCompute,
			D3D12_COMMAND_LIST_TYPE_COMPUTE));
	}

	CopyContext1 =
		std::make_unique<D3D12CommandContext>(this, ED3D12CommandQueueType::Copy1, D3D12_COMMAND_LIST_TYPE_COPY);
	CopyContext2 =
		std::make_unique<D3D12CommandContext>(this, ED3D12CommandQueueType::Copy2, D3D12_COMMAND_LIST_TYPE_COPY);
}

ID3D12Device* D3D12LinkedDevice::GetDevice() const
{
	return GetParentAdapter()->GetD3D12Device();
}

ID3D12Device5* D3D12LinkedDevice::GetDevice5() const
{
	return GetParentAdapter()->GetD3D12Device5();
}

D3D12CommandQueue* D3D12LinkedDevice::GetCommandQueue(ED3D12CommandQueueType Type)
{
	switch (Type)
	{
	case ED3D12CommandQueueType::Direct:
		return &GraphicsQueue;
	case ED3D12CommandQueueType::AsyncCompute:
		return &AsyncComputeQueue;
	case ED3D12CommandQueueType::Copy1:
		return &CopyQueue1;
	case ED3D12CommandQueueType::Copy2:
		return &CopyQueue2;
	default:
		assert(false && "Should not get here");
		return nullptr;
	}
}

D3D12_RESOURCE_ALLOCATION_INFO D3D12LinkedDevice::GetResourceAllocationInfo(const D3D12_RESOURCE_DESC& ResourceDesc)
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

bool D3D12LinkedDevice::ResourceSupport4KBAlignment(D3D12_RESOURCE_DESC& ResourceDesc)
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

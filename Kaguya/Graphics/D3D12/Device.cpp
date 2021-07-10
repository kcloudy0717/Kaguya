#include "pch.h"
#include "Device.h"
#include "Adapter.h"

using Microsoft::WRL::ComPtr;

static AutoConsoleVariable<int> CVar_GlobalResourceViewHeapSize(
	"D3D12.GlobalResourceViewHeapSize",
	"Global Resource View Heap Size",
	4096);

static AutoConsoleVariable<int> CVar_GlobalSamplerHeapSize(
	"D3D12.GlobalSamplerHeapSize",
	"Global Sampler Heap Size",
	D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE);

Device::Device(Adapter* Adapter)
	: AdapterChild(Adapter)
	, GraphicsQueue(this, D3D12_COMMAND_LIST_TYPE_DIRECT, ECommandQueueType::Direct)
	, AsyncComputeQueue(this, D3D12_COMMAND_LIST_TYPE_COMPUTE, ECommandQueueType::AsyncCompute)
	, CopyQueue1(this, D3D12_COMMAND_LIST_TYPE_COPY, ECommandQueueType::Copy1)
	, CopyQueue2(this, D3D12_COMMAND_LIST_TYPE_COPY, ECommandQueueType::Copy2)
	, ResourceDescriptorHeap(this)
	, SamplerDescriptorHeap(this)
	, RenderTargetDescriptorHeap(this)
	, DepthStencilDescriptorHeap(this)
{
}

Device::~Device()
{
}

void Device::Initialize()
{
	GraphicsQueue.Initialize();
	AsyncComputeQueue.Initialize();
	CopyQueue1.Initialize();
	CopyQueue2.Initialize();

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
	return GetParentAdapter()->GetD3D12Device();
}

ID3D12Device5* Device::GetDevice5() const
{
	return GetParentAdapter()->GetD3D12Device5();
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

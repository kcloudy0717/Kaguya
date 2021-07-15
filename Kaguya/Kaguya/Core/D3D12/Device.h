#pragma once
#include <Core/RWLock.h>
#include "D3D12Common.h"
#include "CommandQueue.h"
#include "CommandContext.h"
#include "DescriptorHeap.h"
#include "Resource.h"
#include "Raytracing.h"

// I really want to do mGpu, gotta get myself another 2070 for mGpu
// or get 2 30 series, I can't get one because they're always out of stock >:(
class Device : public AdapterChild
{
public:
	Device(Adapter* Adapter);
	~Device();

	void Initialize();

	ID3D12Device*  GetDevice() const;
	ID3D12Device5* GetDevice5() const;

	CommandQueue* GetCommandQueue(ECommandQueueType Type);
	CommandQueue* GetGraphicsQueue() { return GetCommandQueue(ECommandQueueType::Direct); }
	CommandQueue* GetAsyncComputeQueue() { return GetCommandQueue(ECommandQueueType::AsyncCompute); }
	CommandQueue* GetCopyQueue1() { return GetCommandQueue(ECommandQueueType::Copy1); }
	CommandQueue* GetCopyQueue2() { return GetCommandQueue(ECommandQueueType::Copy2); }

	// clang-format off
	template <typename ViewDesc> DynamicResourceDescriptorHeap& GetDescriptorHeap() noexcept;
	template<> DynamicResourceDescriptorHeap& GetDescriptorHeap<D3D12_SHADER_RESOURCE_VIEW_DESC>() noexcept { return ResourceDescriptorHeap; }
	template<> DynamicResourceDescriptorHeap& GetDescriptorHeap<D3D12_UNORDERED_ACCESS_VIEW_DESC>() noexcept { return ResourceDescriptorHeap; }
	template<> DynamicResourceDescriptorHeap& GetDescriptorHeap<D3D12_RENDER_TARGET_VIEW_DESC>() noexcept { return RenderTargetDescriptorHeap; }
	template<> DynamicResourceDescriptorHeap& GetDescriptorHeap<D3D12_DEPTH_STENCIL_VIEW_DESC>() noexcept { return DepthStencilDescriptorHeap; }
	// clang-format on

	DynamicResourceDescriptorHeap& GetResourceDescriptorHeap() noexcept { return ResourceDescriptorHeap; }
	DynamicResourceDescriptorHeap& GetSamplerDescriptorHeap() noexcept { return SamplerDescriptorHeap; }
	DynamicResourceDescriptorHeap& GetRenderTargetDescriptorHeap() noexcept { return RenderTargetDescriptorHeap; }
	DynamicResourceDescriptorHeap& GetDepthStencilDescriptorHeap() noexcept { return DepthStencilDescriptorHeap; }

	CommandContext& GetCommandContext(UINT ThreadIndex = 0) { return *AvailableCommandContexts[ThreadIndex]; }
	CommandContext& GetAsyncComputeCommandContext(UINT ThreadIndex = 0)
	{
		return *AvailableAsyncCommandContexts[ThreadIndex];
	}
	CommandContext& GetCopyContext1() { return *CopyContext1; }
	CommandContext& GetCopyContext2() { return *CopyContext2; }

	D3D12_RESOURCE_ALLOCATION_INFO GetResourceAllocationInfo(const D3D12_RESOURCE_DESC& ResourceDesc);

	bool ResourceSupport4KBAlignment(D3D12_RESOURCE_DESC& ResourceDesc);

private:
	CommandQueue GraphicsQueue;
	CommandQueue AsyncComputeQueue;
	CommandQueue CopyQueue1;
	CommandQueue CopyQueue2;

	DynamicResourceDescriptorHeap ResourceDescriptorHeap;
	DynamicResourceDescriptorHeap SamplerDescriptorHeap;

	DynamicResourceDescriptorHeap RenderTargetDescriptorHeap;
	DynamicResourceDescriptorHeap DepthStencilDescriptorHeap;

	std::vector<std::unique_ptr<CommandContext>> AvailableCommandContexts;
	std::vector<std::unique_ptr<CommandContext>> AvailableAsyncCommandContexts;

	std::unique_ptr<CommandContext> CopyContext1;
	std::unique_ptr<CommandContext> CopyContext2;

	RWLock													   ResourceAllocationInfoTableLock;
	std::unordered_map<UINT64, D3D12_RESOURCE_ALLOCATION_INFO> ResourceAllocationInfoTable;
};

template<typename ViewDesc>
void Descriptor<ViewDesc>::Allocate()
{
	if (Parent)
	{
		DynamicResourceDescriptorHeap& Heap = Parent->GetDescriptorHeap<ViewDesc>();
		Heap.Allocate(CPUHandle, GPUHandle, Index);
	}
}

template<typename ViewDesc>
void Descriptor<ViewDesc>::Release()
{
	if (Parent && IsValid())
	{
		DynamicResourceDescriptorHeap& Heap = Parent->GetDescriptorHeap<ViewDesc>();
		Heap.Release(Index);
		CPUHandle = { NULL };
		GPUHandle = { NULL };
		Index	  = UINT_MAX;
	}
}

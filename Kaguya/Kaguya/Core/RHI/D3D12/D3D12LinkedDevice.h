#pragma once
#include <Core/RWLock.h>
#include "D3D12Common.h"
#include "D3D12CommandQueue.h"
#include "D3D12CommandContext.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12Resource.h"

class D3D12LinkedDevice : public D3D12DeviceChild
{
public:
	explicit D3D12LinkedDevice(D3D12Device* Parent);
	~D3D12LinkedDevice();

	void Initialize();

	[[nodiscard]] ID3D12Device*	 GetDevice() const;
	[[nodiscard]] ID3D12Device5* GetDevice5() const;

	D3D12CommandQueue* GetCommandQueue(ED3D12CommandQueueType Type);
	D3D12CommandQueue* GetGraphicsQueue() { return GetCommandQueue(ED3D12CommandQueueType::Direct); }
	D3D12CommandQueue* GetAsyncComputeQueue() { return GetCommandQueue(ED3D12CommandQueueType::AsyncCompute); }
	D3D12CommandQueue* GetCopyQueue1() { return GetCommandQueue(ED3D12CommandQueueType::Copy1); }
	D3D12CommandQueue* GetCopyQueue2() { return GetCommandQueue(ED3D12CommandQueueType::Copy2); }

	auto& GetRtvAllocator() noexcept { return RtvAllocator; }
	auto& GetDsvAllocator() noexcept { return DsvAllocator; }

	// clang-format off
	template <typename ViewDesc> D3D12DescriptorHeap& GetDescriptorHeap() noexcept;
	template<> D3D12DescriptorHeap& GetDescriptorHeap<D3D12_SHADER_RESOURCE_VIEW_DESC>() noexcept { return ResourceDescriptorHeap; }
	template<> D3D12DescriptorHeap& GetDescriptorHeap<D3D12_UNORDERED_ACCESS_VIEW_DESC>() noexcept { return ResourceDescriptorHeap; }
	// clang-format on

	D3D12DescriptorHeap& GetResourceDescriptorHeap() noexcept { return ResourceDescriptorHeap; }
	D3D12DescriptorHeap& GetSamplerDescriptorHeap() noexcept { return SamplerDescriptorHeap; }

	D3D12CommandContext& GetCommandContext(UINT ThreadIndex = 0) { return *AvailableCommandContexts[ThreadIndex]; }
	D3D12CommandContext& GetAsyncComputeCommandContext(UINT ThreadIndex = 0)
	{
		return *AvailableAsyncCommandContexts[ThreadIndex];
	}
	D3D12CommandContext& GetCopyContext1() { return *CopyContext1; }
	D3D12CommandContext& GetCopyContext2() { return *CopyContext2; }

	D3D12_RESOURCE_ALLOCATION_INFO GetResourceAllocationInfo(const D3D12_RESOURCE_DESC& ResourceDesc);

	bool ResourceSupport4KbAlignment(D3D12_RESOURCE_DESC& ResourceDesc);

private:
	D3D12CommandQueue GraphicsQueue;
	D3D12CommandQueue AsyncComputeQueue;
	D3D12CommandQueue CopyQueue1;
	D3D12CommandQueue CopyQueue2;

	D3D12DescriptorAllocator RtvAllocator;
	D3D12DescriptorAllocator DsvAllocator;
	D3D12DescriptorHeap		 ResourceDescriptorHeap;
	D3D12DescriptorHeap		 SamplerDescriptorHeap;

	std::vector<std::unique_ptr<D3D12CommandContext>> AvailableCommandContexts;
	std::vector<std::unique_ptr<D3D12CommandContext>> AvailableAsyncCommandContexts;

	std::unique_ptr<D3D12CommandContext> CopyContext1;
	std::unique_ptr<D3D12CommandContext> CopyContext2;

	RWLock													   ResourceAllocationInfoTableLock;
	std::unordered_map<UINT64, D3D12_RESOURCE_ALLOCATION_INFO> ResourceAllocationInfoTable;
};

template<typename ViewDesc>
void D3D12Descriptor<ViewDesc>::Allocate()
{
	if (Parent)
	{
		D3D12DescriptorHeap& DescriptorHeap = Parent->GetDescriptorHeap<ViewDesc>();
		DescriptorHeap.Allocate(CpuHandle, GpuHandle, Index);
	}
}

template<typename ViewDesc>
void D3D12Descriptor<ViewDesc>::Release()
{
	if (Parent && IsValid())
	{
		D3D12DescriptorHeap& DescriptorHeap = Parent->GetDescriptorHeap<ViewDesc>();
		DescriptorHeap.Release(Index);
		CpuHandle = { NULL };
		GpuHandle = { NULL };
		Index	  = UINT_MAX;
	}
}

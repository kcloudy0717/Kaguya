#pragma once
#include <Core/CriticalSection.h>
#include "D3D12Common.h"
#include "D3D12CommandList.h"

struct CommandListBatch
{
	enum
	{
		BatchLimit = 64
	};

	CommandListBatch()
	{
		std::memset(CommandLists, NULL, sizeof(CommandLists));
		NumCommandLists = 0;
	}

	void Add(ID3D12CommandList* CommandList)
	{
		CommandLists[NumCommandLists] = CommandList;
		NumCommandLists++;
	}

	ID3D12CommandList* CommandLists[BatchLimit];
	UINT			   NumCommandLists;
};

class D3D12CommandAllocatorPool : public D3D12LinkedDeviceChild
{
public:
	D3D12CommandAllocatorPool(D3D12LinkedDevice* Parent, D3D12_COMMAND_LIST_TYPE CommandListType) noexcept;

	D3D12CommandAllocator* RequestCommandAllocator();

	void DiscardCommandAllocator(D3D12CommandAllocator* CommandAllocator);

private:
	const D3D12_COMMAND_LIST_TYPE CommandListType;

	std::vector<std::unique_ptr<D3D12CommandAllocator>> CommandAllocators;
	std::queue<D3D12CommandAllocator*>					CommandAllocatorQueue;
	CriticalSection										CriticalSection;
};

class D3D12CommandQueue : public D3D12LinkedDeviceChild
{
public:
	D3D12CommandQueue(D3D12LinkedDevice* Parent, D3D12_COMMAND_LIST_TYPE CommandListType) noexcept;

	void Initialize(ED3D12CommandQueueType CommandQueueType, UINT NumCommandLists = 1);

	[[nodiscard]] ID3D12CommandQueue* GetCommandQueue() const { return CommandQueue.Get(); }
	[[nodiscard]] UINT64			  GetFrequency() const { return Frequency; }
	[[nodiscard]] UINT64			  GetCompletedValue() const { return Fence->GetCompletedValue(); }

	[[nodiscard]] UINT64 AdvanceGpu();

	[[nodiscard]] bool IsFenceComplete(UINT64 FenceValue) const;

	void WaitForFence(UINT64 FenceValue);

	void Wait(D3D12CommandQueue* CommandQueue);
	void WaitForSyncPoint(const D3D12CommandSyncPoint& SyncPoint);

	void Flush() { WaitForFence(AdvanceGpu()); }

	[[nodiscard]] D3D12CommandListHandle RequestCommandList(D3D12CommandAllocator* CommandAllocator)
	{
		D3D12CommandListHandle Handle;
		if (!AvailableCommandListHandles.empty())
		{
			Handle = AvailableCommandListHandles.front();
			AvailableCommandListHandles.pop();

			Handle.Reset(CommandAllocator);
			return Handle;
		}

		return CreateCommandListHandle(CommandAllocator);
	}

	void DiscardCommandList(D3D12CommandListHandle& CommandListHandle)
	{
		AvailableCommandListHandles.push(CommandListHandle);
	}

	void ExecuteCommandLists(
		UINT					NumCommandListHandles,
		D3D12CommandListHandle* CommandListHandles,
		bool					WaitForCompletion);

private:
	bool ResolveResourceBarrierCommandList(
		D3D12CommandListHandle& CommandListHandle,
		D3D12CommandListHandle& ResourceBarrierCommandListHandle);

	D3D12CommandListHandle CreateCommandListHandle(D3D12CommandAllocator* CommandAllocator)
	{
		D3D12CommandListHandle Handle = D3D12CommandListHandle(GetParentLinkedDevice(), CommandListType, this);
		Handle.Reset(CommandAllocator);
		return Handle;
	}

private:
	const D3D12_COMMAND_LIST_TYPE CommandListType;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> CommandQueue;
	UINT64									   Frequency = 0;
	Microsoft::WRL::ComPtr<ID3D12Fence>		   Fence;
	CriticalSection							   FenceMutex;
	UINT64									   FenceValue = 1;
	D3D12CommandSyncPoint					   SyncPoint;

	// Command allocators used exclusively for resolving resource barriers
	D3D12CommandAllocatorPool ResourceBarrierCommandAllocatorPool;
	D3D12CommandAllocator*	  ResourceBarrierCommandAllocator = nullptr;

	std::queue<D3D12CommandListHandle> AvailableCommandListHandles;
};

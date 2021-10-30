#pragma once
#include <Core/CriticalSection.h>
#include "D3D12Common.h"
#include "D3D12CommandList.h"

class D3D12CommandAllocatorPool : public D3D12LinkedDeviceChild
{
public:
	explicit D3D12CommandAllocatorPool(D3D12LinkedDevice* Parent, D3D12_COMMAND_LIST_TYPE CommandListType) noexcept;

	[[nodiscard]] D3D12CommandAllocator* RequestCommandAllocator();

	void DiscardCommandAllocator(D3D12CommandAllocator* CommandAllocator);

private:
	D3D12_COMMAND_LIST_TYPE CommandListType;

	std::vector<std::unique_ptr<D3D12CommandAllocator>> CommandAllocators;
	std::queue<D3D12CommandAllocator*>					CommandAllocatorQueue;
	CriticalSection										CriticalSection;
};

class D3D12CommandQueue : public D3D12LinkedDeviceChild
{
public:
	explicit D3D12CommandQueue(D3D12LinkedDevice* Parent, D3D12_COMMAND_LIST_TYPE CommandListType) noexcept;

	void Initialize(ED3D12CommandQueueType CommandQueueType, UINT NumCommandLists = 1);

	[[nodiscard]] ID3D12CommandQueue* GetCommandQueue() const { return CommandQueue.Get(); }
	[[nodiscard]] UINT64			  GetFrequency() const { return Frequency; }

	[[nodiscard]] UINT64 Signal();

	[[nodiscard]] bool IsFenceComplete(UINT64 FenceValue);

	void HostWaitForValue(UINT64 FenceValue);

	void Wait(D3D12CommandQueue* CommandQueue);
	void WaitForSyncHandle(const D3D12SyncHandle& SyncHandle);

	void WaitIdle() { HostWaitForValue(Signal()); }

	[[nodiscard]] D3D12CommandListHandle RequestCommandList(D3D12CommandAllocator* CommandAllocator);

	void DiscardCommandList(D3D12CommandListHandle&& CommandListHandle);

	void ExecuteCommandLists(
		UINT					NumCommandListHandles,
		D3D12CommandListHandle* CommandListHandles,
		bool					WaitForCompletion);

private:
	[[nodiscard]] bool ResolveResourceBarrierCommandList(
		D3D12CommandListHandle& CommandListHandle,
		D3D12CommandListHandle& ResourceBarrierCommandListHandle);

	[[nodiscard]] D3D12CommandListHandle CreateCommandListHandle(D3D12CommandAllocator* CommandAllocator) const;

private:
	D3D12_COMMAND_LIST_TYPE CommandListType;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> CommandQueue;
	UINT64									   Frequency = 0;
	D3D12Fence								   Fence;
	D3D12SyncHandle							   SyncHandle;

	// Command allocators used exclusively for resolving resource barriers
	D3D12CommandAllocatorPool ResourceBarrierCommandAllocatorPool;
	D3D12CommandAllocator*	  ResourceBarrierCommandAllocator = nullptr;

	std::queue<D3D12CommandListHandle> AvailableCommandListHandles;
};

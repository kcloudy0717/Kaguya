#pragma once
#include "D3D12Core.h"
#include "D3D12Fence.h"
#include "D3D12CommandContext.h"

namespace RHI
{
	class D3D12CommandQueue : public D3D12LinkedDeviceChild
	{
	public:
		D3D12CommandQueue() noexcept = default;
		explicit D3D12CommandQueue(
			D3D12LinkedDevice*		 Parent,
			RHID3D12CommandQueueType Type);

		D3D12CommandQueue(D3D12CommandQueue&&) noexcept = default;
		D3D12CommandQueue& operator=(D3D12CommandQueue&&) noexcept = default;

		D3D12CommandQueue(const D3D12CommandQueue&) noexcept = delete;
		D3D12CommandQueue& operator=(const D3D12CommandQueue&) noexcept = delete;

		[[nodiscard]] D3D12_COMMAND_LIST_TYPE GetType() const noexcept { return CommandListType; }
		[[nodiscard]] ID3D12CommandQueue*	  GetCommandQueue() const noexcept { return CommandQueue.Get(); }
		[[nodiscard]] D3D12Fence&			  GetFence() noexcept { return Fence; }
		[[nodiscard]] D3D12SyncHandle		  GetSyncHandle() const noexcept { return SyncHandle; }
		[[nodiscard]] bool					  SupportTimestamps() const noexcept { return Frequency.has_value(); }
		[[nodiscard]] UINT64				  GetFrequency() const noexcept { return Frequency.value_or(0); }

		[[nodiscard]] UINT64 Signal();

		[[nodiscard]] bool IsFenceComplete(UINT64 FenceValue);

		void HostWaitForValue(UINT64 FenceValue);

		void Wait(D3D12CommandQueue* CommandQueue) const;
		void WaitForSyncHandle(const D3D12SyncHandle& SyncHandle) const;

		void WaitIdle() { HostWaitForValue(Signal()); }

		D3D12SyncHandle ExecuteCommandLists(
			Span<D3D12CommandListHandle* const> CommandListHandles,
			bool								WaitForCompletion);

	private:
		[[nodiscard]] bool ResolveResourceBarrierCommandList(D3D12CommandListHandle* CommandListHandle);

	private:
		D3D12_COMMAND_LIST_TYPE CommandListType;
		Arc<ID3D12CommandQueue> CommandQueue;
		D3D12Fence				Fence;
		D3D12SyncHandle			SyncHandle; // Sync handle updated every ECL

		// Command allocators used exclusively for resolving resource barriers
		D3D12CommandAllocatorPool	ResourceBarrierCommandAllocatorPool;
		Arc<ID3D12CommandAllocator> ResourceBarrierCommandAllocator;
		D3D12CommandListHandle		ResourceBarrierCommandListHandle;

		std::optional<UINT64> Frequency;
	};
} // namespace RHI

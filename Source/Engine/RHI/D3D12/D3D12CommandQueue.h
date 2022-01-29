#pragma once
#include "D3D12Core.h"
#include "D3D12Fence.h"
#include "D3D12CommandList.h"

namespace RHI
{
	class D3D12CommandQueue : public D3D12LinkedDeviceChild
	{
	public:
		explicit D3D12CommandQueue(D3D12LinkedDevice* Parent, RHID3D12CommandQueueType Type);

		[[nodiscard]] ID3D12CommandQueue* GetCommandQueue() const noexcept { return CommandQueue.Get(); }
		[[nodiscard]] UINT64			  GetFrequency() const noexcept { return Frequency; }

		[[nodiscard]] UINT64 Signal();

		[[nodiscard]] bool IsFenceComplete(UINT64 FenceValue);

		void HostWaitForValue(UINT64 FenceValue);

		void Wait(D3D12CommandQueue* CommandQueue);
		void WaitForSyncHandle(const D3D12SyncHandle& SyncHandle);

		void WaitIdle() { HostWaitForValue(Signal()); }

		D3D12SyncHandle ExecuteCommandLists(
			UINT					NumCommandListHandles,
			D3D12CommandListHandle* CommandListHandles,
			bool					WaitForCompletion);

	private:
		Arc<ID3D12CommandQueue> InitializeCommandQueue();
		UINT64					InitializeTimestampFrequency();

		[[nodiscard]] bool ResolveResourceBarrierCommandList(D3D12CommandListHandle& CommandListHandle);

	private:
		D3D12_COMMAND_LIST_TYPE CommandListType;
		Arc<ID3D12CommandQueue> CommandQueue;
		UINT64					Frequency;
		D3D12Fence				Fence;
		D3D12SyncHandle			SyncHandle;

		// Command allocators used exclusively for resolving resource barriers
		D3D12CommandAllocatorPool	ResourceBarrierCommandAllocatorPool;
		Arc<ID3D12CommandAllocator> ResourceBarrierCommandAllocator;
		D3D12CommandListHandle		ResourceBarrierCommandListHandle;
	};

	inline D3D12_COMMAND_LIST_TYPE RHITranslateD3D12(RHID3D12CommandQueueType Type)
	{
		// clang-format off
	switch (Type)
	{
		using enum RHID3D12CommandQueueType;
	case Direct:		return D3D12_COMMAND_LIST_TYPE_DIRECT;
	case AsyncCompute:	return D3D12_COMMAND_LIST_TYPE_COMPUTE;
	case Copy1:			return D3D12_COMMAND_LIST_TYPE_COPY;
	case Copy2:			return D3D12_COMMAND_LIST_TYPE_COPY;
	}
		// clang-format on
		return D3D12_COMMAND_LIST_TYPE();
	}

	inline LPCWSTR GetCommandQueueTypeString(RHID3D12CommandQueueType Type)
	{
		// clang-format off
	switch (Type)
	{
		using enum RHID3D12CommandQueueType;
	case Direct:		return L"3D";
	case AsyncCompute:	return L"Async Compute";
	case Copy1:			return L"Copy 1";
	case Copy2:			return L"Copy 2";
	}
		// clang-format on
		return L"<unknown>";
	}

	inline LPCWSTR GetCommandQueueTypeFenceString(RHID3D12CommandQueueType Type)
	{
		// clang-format off
	switch (Type)
	{
		using enum RHID3D12CommandQueueType;
	case Direct:		return L"3D Fence";
	case AsyncCompute:	return L"Async Compute Fence";
	case Copy1:			return L"Copy 1 Fence";
	case Copy2:			return L"Copy 2 Fence";
	}
		// clang-format on
		return L"<unknown>";
	}
} // namespace RHI

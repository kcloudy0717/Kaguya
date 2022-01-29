#include "D3D12CommandQueue.h"
#include "D3D12LinkedDevice.h"

namespace RHI
{
	D3D12CommandQueue::D3D12CommandQueue(D3D12LinkedDevice* Parent, RHID3D12CommandQueueType Type)
		: D3D12LinkedDeviceChild(Parent)
		, CommandListType(RHITranslateD3D12(Type))
		, CommandQueue(InitializeCommandQueue())
		, Frequency(InitializeTimestampFrequency())
		, Fence(Parent->GetParentDevice(), 0, D3D12_FENCE_FLAG_NONE)
		, ResourceBarrierCommandAllocatorPool(Parent, CommandListType)
		, ResourceBarrierCommandAllocator(ResourceBarrierCommandAllocatorPool.RequestCommandAllocator())
		, ResourceBarrierCommandListHandle(Parent, CommandListType)
	{
#ifdef _DEBUG
		CommandQueue->SetName(GetCommandQueueTypeString(Type));
		Fence.Get()->SetName(GetCommandQueueTypeFenceString(Type));
#endif
	}

	UINT64 D3D12CommandQueue::Signal()
	{
		return Fence.Signal(this);
	}

	bool D3D12CommandQueue::IsFenceComplete(UINT64 FenceValue)
	{
		return Fence.IsFenceComplete(FenceValue);
	}

	void D3D12CommandQueue::HostWaitForValue(UINT64 FenceValue)
	{
		Fence.HostWaitForValue(FenceValue);
	}

	void D3D12CommandQueue::Wait(D3D12CommandQueue* CommandQueue)
	{
		WaitForSyncHandle(CommandQueue->SyncHandle);
	}

	void D3D12CommandQueue::WaitForSyncHandle(const D3D12SyncHandle& SyncHandle)
	{
		if (SyncHandle)
		{
			VERIFY_D3D12_API(CommandQueue->Wait(SyncHandle.Fence->Get(), SyncHandle.Value));
		}
	}

	D3D12SyncHandle D3D12CommandQueue::ExecuteCommandLists(
		UINT					NumCommandListHandles,
		D3D12CommandListHandle* CommandListHandles,
		bool					WaitForCompletion)
	{
		UINT			   NumCommandLists		 = 0;
		UINT			   NumBarrierCommandList = 0;
		ID3D12CommandList* CommandLists[32]		 = {};

		// Resolve resource barriers
		for (UINT i = 0; i < NumCommandListHandles; ++i)
		{
			D3D12CommandListHandle& CommandListHandle = CommandListHandles[i];

			if (ResolveResourceBarrierCommandList(CommandListHandle))
			{
				CommandLists[NumCommandLists++] = ResourceBarrierCommandListHandle.GetCommandList();
				NumBarrierCommandList++;
			}

			CommandLists[NumCommandLists++] = CommandListHandle.GetCommandList();
		}

		CommandQueue->ExecuteCommandLists(NumCommandLists, CommandLists);
		UINT64 FenceValue = Signal();
		SyncHandle		  = D3D12SyncHandle(&Fence, FenceValue);

		// Discard command allocator used exclusively to resolve resource barriers
		if (NumBarrierCommandList > 0)
		{
			ResourceBarrierCommandAllocatorPool.DiscardCommandAllocator(std::exchange(ResourceBarrierCommandAllocator, {}), SyncHandle);
		}

		if (WaitForCompletion)
		{
			HostWaitForValue(FenceValue);
			assert(SyncHandle.IsComplete());
		}

		return SyncHandle;
	}

	Arc<ID3D12CommandQueue> D3D12CommandQueue::InitializeCommandQueue()
	{
		constexpr UINT			 NodeMask = 0;
		Arc<ID3D12CommandQueue>	 CommandQueue;
		D3D12_COMMAND_QUEUE_DESC Desc = {
			.Type	  = CommandListType,
			.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
			.Flags	  = D3D12_COMMAND_QUEUE_FLAG_NONE,
			.NodeMask = NodeMask
		};
		VERIFY_D3D12_API(Parent->GetDevice()->CreateCommandQueue(&Desc, IID_PPV_ARGS(&CommandQueue)));
		return CommandQueue;
	}

	UINT64 D3D12CommandQueue::InitializeTimestampFrequency()
	{
		UINT64 Frequency = 0;
		CommandQueue->GetTimestampFrequency(&Frequency);
		return Frequency;
	}

	bool D3D12CommandQueue::ResolveResourceBarrierCommandList(D3D12CommandListHandle& CommandListHandle)
	{
		std::vector<D3D12_RESOURCE_BARRIER> ResourceBarriers = CommandListHandle.ResolveResourceBarriers();

		bool AnyResolved = !ResourceBarriers.empty();
		if (AnyResolved)
		{
			if (!ResourceBarrierCommandAllocator)
			{
				ResourceBarrierCommandAllocator = ResourceBarrierCommandAllocatorPool.RequestCommandAllocator();
			}

			ResourceBarrierCommandListHandle.Open(ResourceBarrierCommandAllocator);
			ResourceBarrierCommandListHandle->ResourceBarrier(static_cast<UINT>(ResourceBarriers.size()), ResourceBarriers.data());
			ResourceBarrierCommandListHandle.Close();
		}

		return AnyResolved;
	}
} // namespace RHI

#include "D3D12CommandQueue.h"
#include "D3D12LinkedDevice.h"

D3D12CommandAllocatorPool::D3D12CommandAllocatorPool(
	D3D12LinkedDevice*		Parent,
	D3D12_COMMAND_LIST_TYPE CommandListType) noexcept
	: D3D12LinkedDeviceChild(Parent)
	, CommandListType(CommandListType)
{
}

D3D12CommandAllocator* D3D12CommandAllocatorPool::RequestCommandAllocator()
{
	std::scoped_lock _(CriticalSection);

	D3D12CommandAllocator* Allocator = nullptr;

	if (!CommandAllocatorQueue.empty())
	{
		if (auto iter = CommandAllocatorQueue.front(); iter->IsReady())
		{
			Allocator = iter;
			Allocator->Reset();
			CommandAllocatorQueue.pop();
		}
	}

	// If no allocator's were ready to be reused, create a new one
	if (!Allocator)
	{
		Allocator = new D3D12CommandAllocator(GetParentLinkedDevice()->GetDevice(), CommandListType);
		CommandAllocators.push_back(std::unique_ptr<D3D12CommandAllocator>(Allocator));
	}

	return Allocator;
}

void D3D12CommandAllocatorPool::DiscardCommandAllocator(D3D12CommandAllocator* CommandAllocator)
{
	std::scoped_lock _(CriticalSection);
	assert(CommandAllocator->HasValidSyncPoint());
	CommandAllocatorQueue.push(CommandAllocator);
}

D3D12CommandQueue::D3D12CommandQueue(D3D12LinkedDevice* Parent, D3D12_COMMAND_LIST_TYPE CommandListType) noexcept
	: D3D12LinkedDeviceChild(Parent)
	, CommandListType(CommandListType)
	, ResourceBarrierCommandAllocatorPool(Parent, CommandListType)
{
}

void D3D12CommandQueue::Initialize(ED3D12CommandQueueType CommandQueueType, UINT NumCommandLists /*= 1*/)
{
	D3D12LinkedDevice* Device = GetParentLinkedDevice();

	constexpr UINT NodeMask = 0;

	D3D12_COMMAND_QUEUE_DESC Desc = { .Type		= CommandListType,
									  .Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
									  .Flags	= D3D12_COMMAND_QUEUE_FLAG_NONE,
									  .NodeMask = NodeMask };
	VERIFY_D3D12_API(
		Device->GetDevice()->CreateCommandQueue(&Desc, IID_PPV_ARGS(CommandQueue.ReleaseAndGetAddressOf())));
	VERIFY_D3D12_API(
		Device->GetDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(Fence.ReleaseAndGetAddressOf())));

#ifdef _DEBUG
	CommandQueue->SetName(GetCommandQueueTypeString(CommandQueueType));
	Fence->SetName(GetCommandQueueTypeFenceString(CommandQueueType));
#endif

	if (FAILED(CommandQueue->GetTimestampFrequency(&Frequency)))
	{
		Frequency = UINT64_MAX;
	}

	for (UINT i = 0; i < NumCommandLists; ++i)
	{
		AvailableCommandListHandles.push(D3D12CommandListHandle(GetParentLinkedDevice(), CommandListType, this));
	}

	ResourceBarrierCommandAllocator = ResourceBarrierCommandAllocatorPool.RequestCommandAllocator();
}

UINT64 D3D12CommandQueue::AdvanceGpu()
{
	std::scoped_lock _(FenceMutex);
	VERIFY_D3D12_API(CommandQueue->Signal(Fence.Get(), FenceValue));
	return FenceValue++;
}

bool D3D12CommandQueue::IsFenceComplete(UINT64 FenceValue) const
{
	return Fence->GetCompletedValue() >= FenceValue;
}

void D3D12CommandQueue::WaitForFence(UINT64 FenceValue)
{
	if (IsFenceComplete(FenceValue))
	{
		return;
	}

	std::scoped_lock _(FenceMutex);
	VERIFY_D3D12_API(Fence->SetEventOnCompletion(FenceValue, nullptr));
}

void D3D12CommandQueue::Wait(D3D12CommandQueue* CommandQueue)
{
	UINT64 Value = CommandQueue->FenceValue - 1;
	VERIFY_D3D12_API(CommandQueue->GetCommandQueue()->Wait(CommandQueue->Fence.Get(), Value));
}

void D3D12CommandQueue::WaitForSyncPoint(const D3D12CommandSyncPoint& SyncPoint)
{
	if (SyncPoint.IsValid())
	{
		VERIFY_D3D12_API(CommandQueue->Wait(SyncPoint.Fence, SyncPoint.Value));
	}
}

bool D3D12CommandQueue::ResolveResourceBarrierCommandList(
	D3D12CommandListHandle& CommandListHandle,
	D3D12CommandListHandle& ResourceBarrierCommandListHandle)
{
	std::vector<D3D12_RESOURCE_BARRIER> ResourceBarriers = CommandListHandle.ResolveResourceBarriers();
	UINT								NumBarriers		 = static_cast<UINT>(ResourceBarriers.size());

	bool AnyResolved = NumBarriers > 0;
	if (AnyResolved)
	{
		if (!ResourceBarrierCommandAllocator)
		{
			ResourceBarrierCommandAllocator = ResourceBarrierCommandAllocatorPool.RequestCommandAllocator();
		}

		ResourceBarrierCommandListHandle = RequestCommandList(ResourceBarrierCommandAllocator);
		ResourceBarrierCommandListHandle->ResourceBarrier(NumBarriers, ResourceBarriers.data());
	}

	return AnyResolved;
}

void D3D12CommandQueue::ExecuteCommandLists(
	UINT					NumCommandListHandles,
	D3D12CommandListHandle* CommandListHandles,
	bool					WaitForCompletion)
{
	D3D12CommandSyncPoint  SyncPoint;
	CommandListBatch	   Batch;
	D3D12CommandListHandle BarrierCommandList[64];
	UINT				   NumBarrierCommandList = 0;

	for (UINT i = 0; i < NumCommandListHandles; ++i)
	{
		D3D12CommandListHandle& hCmdList = CommandListHandles[i];
		D3D12CommandListHandle	hBarrierCmdList;

		bool Resolved = ResolveResourceBarrierCommandList(hCmdList, hBarrierCmdList);
		if (Resolved)
		{
			BarrierCommandList[NumBarrierCommandList++] = hBarrierCmdList;

			hBarrierCmdList.Close();

			Batch.Add(hBarrierCmdList.GetCommandList());
		}

		Batch.Add(hCmdList.GetCommandList());
	}

	CommandQueue->ExecuteCommandLists(Batch.NumCommandLists, Batch.CommandLists);
	UINT64 FenceValue = AdvanceGpu();
	SyncPoint		  = D3D12CommandSyncPoint(Fence.Get(), FenceValue);

	for (UINT i = 0; i < NumCommandListHandles; ++i)
	{
		D3D12CommandListHandle& hCmdList = CommandListHandles[i];
		hCmdList.SetSyncPoint(SyncPoint);

		DiscardCommandList(hCmdList);
	}

	for (UINT i = 0; i < NumBarrierCommandList; ++i)
	{
		D3D12CommandListHandle& hCmdList = BarrierCommandList[i];
		hCmdList.SetSyncPoint(SyncPoint);

		DiscardCommandList(hCmdList);
	}

	if (NumBarrierCommandList > 0)
	{
		ResourceBarrierCommandAllocator->SetSyncPoint(SyncPoint);
		ResourceBarrierCommandAllocatorPool.DiscardCommandAllocator(ResourceBarrierCommandAllocator);
		ResourceBarrierCommandAllocator = nullptr;
	}

	if (WaitForCompletion)
	{
		WaitForFence(FenceValue);
		assert(SyncPoint.IsComplete());
	}
}

#include "CommandQueue.h"
#include "Device.h"

CommandAllocatorPool::CommandAllocatorPool(Device* Device, D3D12_COMMAND_LIST_TYPE CommandListType) noexcept
	: DeviceChild(Device)
	, CommandListType(CommandListType)
{
}

CommandAllocator* CommandAllocatorPool::RequestCommandAllocator()
{
	std::scoped_lock _(CriticalSection);

	CommandAllocator* Allocator = nullptr;

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
		Allocator = new CommandAllocator(GetParentDevice()->GetDevice(), CommandListType);
		CommandAllocators.push_back(std::unique_ptr<CommandAllocator>(Allocator));
	}

	return Allocator;
}

void CommandAllocatorPool::DiscardCommandAllocator(CommandAllocator* CommandAllocator)
{
	std::scoped_lock _(CriticalSection);
	assert(CommandAllocator->HasValidSyncPoint());
	CommandAllocatorQueue.push(CommandAllocator);
}

CommandQueue::CommandQueue(Device* Device, D3D12_COMMAND_LIST_TYPE CommandListType, ECommandQueueType CommandQueueType)
	: DeviceChild(Device)
	, CommandListType(CommandListType)
	, CommandQueueType(CommandQueueType)
	, ResourceBarrierCommandAllocatorPool(Device, CommandListType)
{
}

void CommandQueue::Initialize(std::optional<UINT> NumCommandLists /*= {}*/)
{
	Device* Device = GetParentDevice();

	constexpr UINT NodeMask = 0;

	D3D12_COMMAND_QUEUE_DESC Desc = { .Type		= CommandListType,
									  .Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
									  .Flags	= D3D12_COMMAND_QUEUE_FLAG_NONE,
									  .NodeMask = NodeMask };
	ASSERTD3D12APISUCCEEDED(
		Device->GetDevice()->CreateCommandQueue(&Desc, IID_PPV_ARGS(pCommandQueue.ReleaseAndGetAddressOf())));

	pCommandQueue->SetName(GetCommandQueueTypeString(CommandQueueType));

	ASSERTD3D12APISUCCEEDED(
		Device->GetDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(Fence.ReleaseAndGetAddressOf())));

	FenceValue = 1;

	if (FAILED(pCommandQueue->GetTimestampFrequency(&Frequency)))
	{
		Frequency = UINT64_MAX;
	}

	if (NumCommandLists)
	{
		for (UINT i = 0; i < *NumCommandLists; ++i)
		{
			AvailableCommandListHandles.push(CommandListHandle(GetParentDevice(), CommandListType, this));
		}
	}

	ResourceBarrierCommandAllocator = ResourceBarrierCommandAllocatorPool.RequestCommandAllocator();
}

UINT64 CommandQueue::AdvanceGpu()
{
	std::scoped_lock _(FenceMutex);
	ASSERTD3D12APISUCCEEDED(pCommandQueue->Signal(Fence.Get(), FenceValue));
	return FenceValue++;
}

bool CommandQueue::IsFenceComplete(UINT64 FenceValue)
{
	return Fence->GetCompletedValue() >= FenceValue;
}

void CommandQueue::WaitForFence(UINT64 FenceValue)
{
	if (IsFenceComplete(FenceValue))
	{
		return;
	}

	std::scoped_lock _(FenceMutex);
	ASSERTD3D12APISUCCEEDED(Fence->SetEventOnCompletion(FenceValue, nullptr));
}

void CommandQueue::Wait(CommandQueue* CommandQueue)
{
	UINT64 Value = CommandQueue->FenceValue - 1;
	ASSERTD3D12APISUCCEEDED(pCommandQueue->Wait(CommandQueue->Fence.Get(), Value));
}

void CommandQueue::WaitForSyncPoint(const CommandSyncPoint& SyncPoint)
{
	if (SyncPoint.IsValid())
	{
		ASSERTD3D12APISUCCEEDED(pCommandQueue->Wait(SyncPoint.Fence, SyncPoint.Value));
	}
}

bool CommandQueue::ResolveResourceBarrierCommandList(
	CommandListHandle& hCmdList,
	CommandListHandle& hResourceBarrierCmdList)
{
	std::vector<PendingResourceBarrier>& PendingResourceBarriers = hCmdList.GetPendingResourceBarriers();

	std::vector<D3D12_RESOURCE_BARRIER> ResourceBarriers;
	ResourceBarriers.reserve(PendingResourceBarriers.size());

	D3D12_RESOURCE_BARRIER Desc = {};
	Desc.Type					= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	for (const auto& Pending : PendingResourceBarriers)
	{
		CResourceState& ResourceState = Pending.Resource->GetResourceState();

		Desc.Transition.Subresource = Pending.Subresource;

		const D3D12_RESOURCE_STATES StateBefore = ResourceState.GetSubresourceState(Pending.Subresource);
		const D3D12_RESOURCE_STATES StateAfter =
			(Pending.State != D3D12_RESOURCE_STATE_UNKNOWN) ? Pending.State : StateBefore;

		if (StateBefore != StateAfter)
		{
			Desc.Transition.pResource	= Pending.Resource->GetResource();
			Desc.Transition.StateBefore = StateBefore;
			Desc.Transition.StateAfter	= StateAfter;

			ResourceBarriers.push_back(Desc);
		}

		const D3D12_RESOURCE_STATES StateCommandList =
			hCmdList.GetResourceState(Pending.Resource).GetSubresourceState(Desc.Transition.Subresource);
		const D3D12_RESOURCE_STATES StatePrevious =
			(StateCommandList != D3D12_RESOURCE_STATE_UNKNOWN) ? StateCommandList : StateAfter;

		if (StateBefore != StatePrevious)
		{
			ResourceState.SetSubresourceState(Desc.Transition.Subresource, StatePrevious);
		}
	}

	UINT NumBarriers = static_cast<UINT>(ResourceBarriers.size());
	if (NumBarriers > 0)
	{
		if (!ResourceBarrierCommandAllocator)
		{
			ResourceBarrierCommandAllocator = ResourceBarrierCommandAllocatorPool.RequestCommandAllocator();
		}

		hResourceBarrierCmdList = RequestCommandList(ResourceBarrierCommandAllocator);

		hResourceBarrierCmdList->ResourceBarrier(NumBarriers, ResourceBarriers.data());
	}

	return NumBarriers > 0;
}

void CommandQueue::ExecuteCommandLists(
	UINT			   NumCommandListHandles,
	CommandListHandle* CommandListHandles,
	bool			   WaitForCompletion)
{
	CommandSyncPoint  SyncPoint;
	CommandListBatch  Batch;
	CommandListHandle BarrierCommandList[64];
	UINT			  NumBarrierCommandList = 0;

	for (UINT i = 0; i < NumCommandListHandles; ++i)
	{
		CommandListHandle& hCmdList = CommandListHandles[i];
		CommandListHandle  hBarrierCmdList;

		bool Resolved = ResolveResourceBarrierCommandList(hCmdList, hBarrierCmdList);
		if (Resolved)
		{
			BarrierCommandList[NumBarrierCommandList++] = hBarrierCmdList;

			hBarrierCmdList.Close();

			Batch.Add(hBarrierCmdList.GetCommandList());
		}

		Batch.Add(hCmdList.GetCommandList());
	}

	pCommandQueue->ExecuteCommandLists(Batch.NumCommandLists, Batch.CommandLists);
	UINT64 FenceValue = AdvanceGpu();
	SyncPoint		  = CommandSyncPoint(Fence.Get(), FenceValue);

	for (UINT i = 0; i < NumCommandListHandles; ++i)
	{
		CommandListHandle& hCmdList = CommandListHandles[i];
		hCmdList.SetSyncPoint(SyncPoint);

		DiscardCommandList(hCmdList);
	}

	for (UINT i = 0; i < NumBarrierCommandList; ++i)
	{
		CommandListHandle& hCmdList = BarrierCommandList[i];
		hCmdList.SetSyncPoint(SyncPoint);

		DiscardCommandList(hCmdList);
	}

	if (WaitForCompletion)
	{
		WaitForFence(FenceValue);
		assert(SyncPoint.IsComplete());
	}
}

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
	assert(CommandAllocator->HasValidSyncHandle());
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

	D3D12_COMMAND_QUEUE_DESC Desc = {};
	Desc.Type					  = CommandListType;
	Desc.Priority				  = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	Desc.Flags					  = D3D12_COMMAND_QUEUE_FLAG_NONE;
	Desc.NodeMask				  = NodeMask;

	VERIFY_D3D12_API(
		Device->GetDevice()->CreateCommandQueue(&Desc, IID_PPV_ARGS(CommandQueue.ReleaseAndGetAddressOf())));
	VERIFY_D3D12_API(
		Device->GetDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(Fence.ReleaseAndGetAddressOf())));

	CommandQueue->SetName(GetCommandQueueTypeString(CommandQueueType));
#ifdef _DEBUG
	Fence->SetName(GetCommandQueueTypeFenceString(CommandQueueType));
#endif

	if (FAILED(CommandQueue->GetTimestampFrequency(&Frequency)))
	{
		Frequency = UINT64_MAX;
	}

	for (UINT i = 0; i < NumCommandLists; ++i)
	{
		AvailableCommandListHandles.push(D3D12CommandListHandle(GetParentLinkedDevice(), CommandListType));
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

void D3D12CommandQueue::HostWaitForValue(UINT64 FenceValue)
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
	WaitForSyncHandle(CommandQueue->SyncHandle);
}

void D3D12CommandQueue::WaitForSyncHandle(const D3D12SyncHandle& SyncHandle)
{
	if (SyncHandle.IsValid())
	{
		VERIFY_D3D12_API(CommandQueue->Wait(SyncHandle.Fence, SyncHandle.Value));
	}
}

D3D12CommandListHandle D3D12CommandQueue::RequestCommandList(D3D12CommandAllocator* CommandAllocator)
{
	if (!AvailableCommandListHandles.empty())
	{
		D3D12CommandListHandle Handle = std::move(AvailableCommandListHandles.front());
		AvailableCommandListHandles.pop();

		Handle.Reset(CommandAllocator);
		return Handle;
	}

	return CreateCommandListHandle(CommandAllocator);
}

void D3D12CommandQueue::DiscardCommandList(D3D12CommandListHandle&& CommandListHandle)
{
	AvailableCommandListHandles.push(std::move(CommandListHandle));
}

bool D3D12CommandQueue::ResolveResourceBarrierCommandList(
	D3D12CommandListHandle& CommandListHandle,
	D3D12CommandListHandle& ResourceBarrierCommandListHandle)
{
	std::vector<D3D12_RESOURCE_BARRIER> ResourceBarriers = CommandListHandle.ResolveResourceBarriers();

	bool AnyResolved = !ResourceBarriers.empty();
	if (AnyResolved)
	{
		if (!ResourceBarrierCommandAllocator)
		{
			ResourceBarrierCommandAllocator = ResourceBarrierCommandAllocatorPool.RequestCommandAllocator();
		}

		ResourceBarrierCommandListHandle = RequestCommandList(ResourceBarrierCommandAllocator);
		ResourceBarrierCommandListHandle->ResourceBarrier(
			static_cast<UINT>(ResourceBarriers.size()),
			ResourceBarriers.data());
		ResourceBarrierCommandListHandle.Close();
	}

	return AnyResolved;
}

D3D12CommandListHandle D3D12CommandQueue::CreateCommandListHandle(D3D12CommandAllocator* CommandAllocator) const
{
	D3D12CommandListHandle Handle(GetParentLinkedDevice(), CommandListType);
	Handle.Reset(CommandAllocator);
	return Handle;
}

void D3D12CommandQueue::ExecuteCommandLists(
	UINT					NumCommandListHandles,
	D3D12CommandListHandle* CommandListHandles,
	bool					WaitForCompletion)
{
	UINT			   NumCommandLists	= 0;
	ID3D12CommandList* CommandLists[64] = {};

	UINT				   NumBarrierCommandList   = 0;
	D3D12CommandListHandle BarrierCommandLists[64] = {};

	// Resolve resource barriers
	for (UINT i = 0; i < NumCommandListHandles; ++i)
	{
		D3D12CommandListHandle& CommandListHandle = CommandListHandles[i];
		D3D12CommandListHandle	BarrierCommandListHandle;

		if (ResolveResourceBarrierCommandList(CommandListHandle, BarrierCommandListHandle))
		{
			CommandLists[NumCommandLists++]				 = BarrierCommandListHandle.GetCommandList();
			BarrierCommandLists[NumBarrierCommandList++] = std::move(BarrierCommandListHandle);
		}

		CommandLists[NumCommandLists++] = CommandListHandle.GetCommandList();
	}

	CommandQueue->ExecuteCommandLists(NumCommandLists, CommandLists);
	UINT64 FenceValue = AdvanceGpu();
	SyncHandle		  = D3D12SyncHandle(Fence.Get(), FenceValue);

	// Discard command lists
	for (UINT i = 0; i < NumCommandListHandles; ++i)
	{
		D3D12CommandListHandle& CommandListHandle = CommandListHandles[i];
		CommandListHandle.SetSyncPoint(SyncHandle);
		DiscardCommandList(std::move(CommandListHandle));
	}
	for (UINT i = 0; i < NumBarrierCommandList; ++i)
	{
		D3D12CommandListHandle& CommandListHandle = BarrierCommandLists[i];
		CommandListHandle.SetSyncPoint(SyncHandle);
		DiscardCommandList(std::move(CommandListHandle));
	}

	// Discard command allocator used exclusively to resolve resource barriers
	if (NumBarrierCommandList > 0)
	{
		ResourceBarrierCommandAllocator->SetSyncPoint(SyncHandle);
		ResourceBarrierCommandAllocatorPool.DiscardCommandAllocator(std::exchange(ResourceBarrierCommandAllocator, {}));
	}

	if (WaitForCompletion)
	{
		HostWaitForValue(FenceValue);
		assert(SyncHandle.IsComplete());
	}
}

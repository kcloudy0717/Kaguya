#include "pch.h"
#include "BuddyAllocator.h"
#include "Device.h"

BuddyAllocator::BuddyAllocator(
	Device*				Device,
	EAllocationStrategy AllocationStrategy,
	D3D12_HEAP_TYPE		HeapType,
	UINT				MaxBlockSize,
	UINT				MinBlockSize /*= D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT*/)
	: DeviceChild(Device)
	, AllocationStrategy(AllocationStrategy)
	, HeapType(HeapType)
	, MaxBlockSize(MaxBlockSize)
	, MinBlockSize(MinBlockSize)
{
	assert(IsDivisible(MaxBlockSize, MinBlockSize));
	assert(IsPowerOfTwo(MaxBlockSize / MinBlockSize));

	MaxOrder = UnitSizeToOrder(SizeToUnitSize(MaxBlockSize));

	Reset();
}

void BuddyAllocator::Initialize()
{
	D3D12_HEAP_PROPERTIES HeapProperties = CD3DX12_HEAP_PROPERTIES(HeapType);

	if (AllocationStrategy == EAllocationStrategy::PlacedResource)
	{
		D3D12_HEAP_DESC Desc = {};
		Desc.SizeInBytes	 = MaxBlockSize;
		Desc.Properties		 = HeapProperties;
		Desc.Alignment		 = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		Desc.Flags			 = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;

		GetParentDevice()->GetDevice()->CreateHeap(&Desc, IID_PPV_ARGS(Heap.ReleaseAndGetAddressOf()));
	}
	else
	{
		D3D12_RESOURCE_DESC Desc = CD3DX12_RESOURCE_DESC::Buffer(MaxBlockSize);
		GetParentDevice()->GetDevice()->CreateCommittedResource(
			&HeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&Desc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(Buffer.ReleaseAndGetAddressOf()));
	}
}

BuddyBlock* BuddyAllocator::Allocate(UINT SizeInBytes)
{
	const UINT UnitSize = SizeToUnitSize(SizeInBytes);
	const UINT Order	= UnitSizeToOrder(UnitSize);
	const UINT Offset	= AllocateBlock(Order);

	UINT AllocatedSize		  = OrderToUnitSize(Order) * MinBlockSize;
	UINT AllocatedBlockOffset = Offset * MinBlockSize;

	BuddyBlock* pBlock	 = new BuddyBlock();
	pBlock->Offset		 = AllocatedBlockOffset;
	pBlock->Size		 = AllocatedSize;
	pBlock->UnpaddedSize = SizeInBytes;
	pBlock->FenceValue	 = 0;

	if (AllocationStrategy == EAllocationStrategy::PlacedResource)
	{
		// TODO: Implement
		__debugbreak();
	}
	else
	{
		pBlock->Buffer = Buffer.Get();
	}

	return pBlock;
}

void BuddyAllocator::Deallocate(BuddyBlock* pBlock)
{
	// TODO: Implement
	__debugbreak();
}

void BuddyAllocator::CleanUpAllocations()
{
	// TODO: Implement
	__debugbreak();
	// while (DeferredDeletionQueue.empty() == false &&
	//	g_CommandManager.IsFenceComplete(m_deferredDeletionQueue.front()->m_fenceValue))
	//{
	//	BuddyBlock* pBlock = m_deferredDeletionQueue.front();
	//	m_deferredDeletionQueue.pop();
	//
	//	DeallocateInternal(pBlock);
	//}
}

UINT BuddyAllocator::AllocateBlock(UINT order)
{
	UINT offset;

	if (order > MaxOrder)
	{
		throw std::bad_alloc(); // Can't allocate a block that large
	}

	auto it = FreeBlocks[order].begin();
	if (it == FreeBlocks[order].end())
	{
		// No free nodes in the requested pool.  Try to find a higher-order block and split it.
		UINT left = AllocateBlock(order + 1);

		UINT size = OrderToUnitSize(order);

		UINT right = left + size;

		FreeBlocks[order].insert(right); // Add the right block to the free pool

		offset = left; // Return the left block
	}
	else
	{
		offset = static_cast<UINT>(*it);

		// Remove the block from the free list
		FreeBlocks[order].erase(it);
	}

	return offset;
}

void BuddyAllocator::DeallocateBlock(UINT offset, UINT order)
{
	// See if the buddy block is free
	UINT size = OrderToUnitSize(order);

	UINT buddy = GetBuddyOffset(offset, size);

	auto it = FreeBlocks[order].find(buddy);
	if (it != FreeBlocks[order].end())
	{
		// Deallocate merged blocks
		DeallocateBlock(std::min(offset, buddy), order + 1);
		// Remove the buddy from the free list
		FreeBlocks[order].erase(it);
	}
	else
	{
		// Add the block to the free list
		FreeBlocks[order].insert(offset); // throw(std::bad_alloc)
	}
}

void BuddyAllocator::DeallocateInternal(BuddyBlock* pBlock)
{
	UINT Offset = SizeToUnitSize(pBlock->Offset);
	UINT Size	= SizeToUnitSize(pBlock->Size);
	UINT Order	= UnitSizeToOrder(Size);

	try
	{
		DeallocateBlock(Offset, Order); // throw(std::bad_alloc)

		if (AllocationStrategy == EAllocationStrategy::PlacedResource)
		{
			// Release the resource
			pBlock->Buffer->Release();
		}
		delete pBlock;
	}
	catch (std::bad_alloc&)
	{
		// Deallocate failed trying to add the free block to the pool
		// resulting in a leak.  Unfortunately there is not much we can do.
		// Fortunately this is expected to be extremely rare as the storage
		// needed for each deallocate is very small.
	}
}

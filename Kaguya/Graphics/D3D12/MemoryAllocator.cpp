#include "pch.h"
#include "MemoryAllocator.h"
#include "Device.h"

std::optional<Allocation> LinearAllocatorPage::Suballocate(UINT64 Size, UINT Alignment)
{
	UINT64 NewOffset = AlignUp(Offset, static_cast<UINT64>(Alignment));
	if (NewOffset + Size > this->PageSize)
	{
		return {};
	}

	Allocation Allocation = { .pResource		 = pResource.Get(),
							  .Offset			 = Offset,
							  .Size				 = Size,
							  .CPUVirtualAddress = CPUVirtualAddress + NewOffset,
							  .GPUVirtualAddress = GPUVirtualAddress + NewOffset };

	Offset = NewOffset + Size;

	return Allocation;
}

void LinearAllocatorPage::Reset()
{
	Offset = 0;
}

LinearAllocator::LinearAllocator(ID3D12Device* Device)
	: Device(Device)
	, CompletedFenceValue(0)
	, CurrentPage(nullptr)
{
}

void LinearAllocator::Begin(UINT64 CompletedFenceValue)
{
	this->CompletedFenceValue = CompletedFenceValue;
}

void LinearAllocator::End(UINT64 FenceValue)
{
	if (!CurrentPage)
	{
		return;
	}

	RetiredPageList.push_back(CurrentPage);
	CurrentPage = nullptr;

	DiscardPages(FenceValue, RetiredPageList);
	RetiredPageList.clear();
}

Allocation LinearAllocator::Allocate(UINT64 Size, UINT Alignment /*= DefaultAlignment*/)
{
	if (!CurrentPage)
	{
		CurrentPage = RequestPage(CompletedFenceValue);
	}

	std::optional<Allocation> OptAllocation = CurrentPage->Suballocate(Size, Alignment);

	if (!OptAllocation)
	{
		RetiredPageList.push_back(CurrentPage);

		CurrentPage	  = RequestPage(CompletedFenceValue);
		OptAllocation = CurrentPage->Suballocate(Size, Alignment);
		assert(OptAllocation.has_value());
	}

	return OptAllocation.value();
}

LinearAllocatorPage* LinearAllocator::RequestPage(UINT64 CompletedFenceValue)
{
	std::scoped_lock _(CriticalSection);

	while (!RetiredPages.empty() && RetiredPages.front().first <= CompletedFenceValue)
	{
		AvailablePages.push(RetiredPages.front().second);
		RetiredPages.pop();
	}

	LinearAllocatorPage* pPage = nullptr;

	if (!AvailablePages.empty())
	{
		pPage = AvailablePages.front();
		pPage->Reset();
		AvailablePages.pop();
	}
	else
	{
		pPage = CreateNewPage(CpuAllocatorPageSize);
		PagePool.emplace_back(pPage);
	}

	return pPage;
}

LinearAllocatorPage* LinearAllocator::CreateNewPage(UINT64 PageSize)
{
	auto HeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto ResourceDesc	= CD3DX12_RESOURCE_DESC::Buffer(PageSize);

	Microsoft::WRL::ComPtr<ID3D12Resource> pResource;
	ASSERTD3D12APISUCCEEDED(Device->CreateCommittedResource(
		&HeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&ResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(pResource.ReleaseAndGetAddressOf())));

#ifdef _DEBUG
	pResource->SetName(L"Linear Allocator Page");
#endif

	return new LinearAllocatorPage(pResource, PageSize);
}

void LinearAllocator::DiscardPages(UINT64 FenceValue, const std::vector<LinearAllocatorPage*>& Pages)
{
	std::scoped_lock _(CriticalSection);
	for (const auto& Page : Pages)
	{
		RetiredPages.push(std::make_pair(FenceValue, Page));
	}
}

D3D12_RESOURCE_STATES GetBufferInitialState(D3D12_HEAP_TYPE HeapType)
{
	switch (HeapType)
	{
	case D3D12_HEAP_TYPE_DEFAULT:
		return D3D12_RESOURCE_STATE_COMMON;
	case D3D12_HEAP_TYPE_UPLOAD:
		return D3D12_RESOURCE_STATE_GENERIC_READ;
	case D3D12_HEAP_TYPE_READBACK:
		return D3D12_RESOURCE_STATE_COPY_DEST;
	default:
		assert("Should not get here" && false);
		return D3D12_RESOURCE_STATE_COMMON;
	}
}

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

		ASSERTD3D12APISUCCEEDED(
			GetParentDevice()->GetDevice()->CreateHeap(&Desc, IID_PPV_ARGS(Heap.ReleaseAndGetAddressOf())));
	}
	else
	{
		D3D12_RESOURCE_DESC Desc = CD3DX12_RESOURCE_DESC::Buffer(MaxBlockSize);
		ASSERTD3D12APISUCCEEDED(GetParentDevice()->GetDevice()->CreateCommittedResource(
			&HeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&Desc,
			GetBufferInitialState(HeapType),
			nullptr,
			IID_PPV_ARGS(Buffer.ReleaseAndGetAddressOf())));
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

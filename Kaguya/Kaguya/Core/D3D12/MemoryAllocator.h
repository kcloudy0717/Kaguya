#pragma once
#include <Core/CoreDefines.h>
#include <Core/CriticalSection.h>
#include "D3D12Common.h"

struct Allocation
{
	ID3D12Resource*			  pResource;
	UINT64					  Offset;
	UINT64					  Size;
	BYTE*					  CPUVirtualAddress;
	D3D12_GPU_VIRTUAL_ADDRESS GPUVirtualAddress;
};

class LinearAllocatorPage
{
public:
	LinearAllocatorPage(Microsoft::WRL::ComPtr<ID3D12Resource> pResource, UINT64 PageSize)
		: pResource(pResource)
		, Offset(0)
		, PageSize(PageSize)
	{
		pResource->Map(0, nullptr, reinterpret_cast<void**>(&CPUVirtualAddress));
		GPUVirtualAddress = pResource->GetGPUVirtualAddress();
	}

	~LinearAllocatorPage() { pResource->Unmap(0, nullptr); }

	std::optional<Allocation> Suballocate(UINT64 Size, UINT Alignment);

	void Reset();

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> pResource;
	UINT64								   Offset;
	UINT64								   PageSize;
	BYTE*								   CPUVirtualAddress;
	D3D12_GPU_VIRTUAL_ADDRESS			   GPUVirtualAddress;
};

class LinearAllocator
{
public:
	enum
	{
		DefaultAlignment = 256,

		CpuAllocatorPageSize = 128 * 1024
	};

	LinearAllocator(ID3D12Device* Device);

	void End(CommandSyncPoint SyncPoint);

	Allocation Allocate(UINT64 Size, UINT Alignment = DefaultAlignment);

	template<typename T>
	Allocation Allocate(const T& Data, UINT Alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT)
	{
		Allocation Allocation = Allocate(sizeof(T), Alignment);
		std::memcpy(Allocation.CPUVirtualAddress, &Data, sizeof(T));
		return Allocation;
	}

private:
	LinearAllocatorPage* RequestPage();

	LinearAllocatorPage* CreateNewPage(UINT64 PageSize);

	void DiscardPages(UINT64 FenceValue, const std::vector<LinearAllocatorPage*>& Pages);

private:
	ID3D12Device* Device;

	std::vector<std::unique_ptr<LinearAllocatorPage>>	PagePool;
	std::queue<std::pair<UINT64, LinearAllocatorPage*>> RetiredPages;
	std::queue<LinearAllocatorPage*>					AvailablePages;
	CriticalSection										CriticalSection;

	CommandSyncPoint				  SyncPoint;
	LinearAllocatorPage*			  CurrentPage;
	std::vector<LinearAllocatorPage*> RetiredPageList;
};

// Allocates blocks from a fixed range using buddy allocation method.
// Buddy allocation allows reasonably fast allocation of arbitrary size blocks
// with minimal fragmentation and provides efficient reuse of freed ranges.
// When a block is de-allocated an attempt is made to merge it with it's
// neighbour (buddy) if it is contiguous and free.
// Based on reference implementation by Bill Kristiansen

enum class EAllocationStrategy
{
	// This strategy uses Placed Resources to sub-allocate a buffer out of an underlying ID3D12Heap.
	// The benefit of this is that each buffer can have it's own resource state and can be treated
	// as any other buffer. The downside of this strategy is the API limitiation which enforces
	// the minimum buffer size to 64k leading to large internal fragmentation in the allocator
	PlacedResource,
	// The alternative is to manualy sub-allocate out of a single large buffer which allows block
	// allocation granularity down to 1 byte. However, this strategy is only really valid for buffers which
	// will be treated as read-only after their creation (i.e. most Index and Vertex buffers). This
	// is because the underlying resource can only have one state at a time.
	ManualSubAllocation
};

struct BuddyBlock
{
	ID3D12Heap*		Heap;
	ID3D12Resource* Buffer;

	UINT   Offset;
	UINT   Size;
	UINT   UnpaddedSize;
	UINT64 FenceValue;
};

class BuddyAllocator : public DeviceChild
{
public:
	BuddyAllocator(
		Device*				Device,
		EAllocationStrategy AllocationStrategy,
		D3D12_HEAP_TYPE		HeapType,
		UINT				MaxBlockSize,
		UINT				MinBlockSize = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

	void Initialize();

	inline void Reset()
	{
		// Clear the free blocks collection
		FreeBlocks.clear();

		// Initialize the pool with a free inner block of max inner block size
		FreeBlocks.resize(MaxOrder + 1);
		FreeBlocks[MaxOrder].insert((size_t)0);
	}

	BuddyBlock* Allocate(UINT SizeInBytes);

	void Deallocate(BuddyBlock* pBlock);

	void CleanUpAllocations();

private:
	inline UINT SizeToUnitSize(UINT size) const { return (size + (MinBlockSize - 1)) / MinBlockSize; }

	inline UINT UnitSizeToOrder(UINT size) const
	{
		return Log2(size); // Log2 rounds up fractions to next whole value
	}

	inline UINT GetBuddyOffset(UINT offset, UINT size) { return offset ^ size; }

	UINT OrderToUnitSize(UINT order) const { return ((UINT)1) << order; }
	UINT AllocateBlock(UINT order);
	void DeallocateBlock(UINT offset, UINT order);

	void DeallocateInternal(BuddyBlock* pBlock);

private:
	const EAllocationStrategy AllocationStrategy;
	const D3D12_HEAP_TYPE	  HeapType;
	const UINT				  MaxBlockSize;
	const UINT				  MinBlockSize;

	Microsoft::WRL::ComPtr<ID3D12Heap>	   Heap;
	Microsoft::WRL::ComPtr<ID3D12Resource> Buffer;

	std::queue<BuddyBlock*>		  DeferredDeletionQueue;
	std::vector<std::set<size_t>> FreeBlocks;
	UINT						  MaxOrder;
};

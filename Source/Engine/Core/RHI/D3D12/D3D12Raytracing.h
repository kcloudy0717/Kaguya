#pragma once
#include "D3D12Common.h"

class D3D12RaytracingGeometry
{
public:
	D3D12RaytracingGeometry() noexcept = default;
	explicit D3D12RaytracingGeometry(size_t Size) { RaytracingGeometryDescs.reserve(Size); }

	auto begin() noexcept { return RaytracingGeometryDescs.begin(); }
	auto end() noexcept { return RaytracingGeometryDescs.end(); }

	[[nodiscard]] UINT Size() const noexcept { return static_cast<UINT>(RaytracingGeometryDescs.size()); }

	[[nodiscard]] D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS GetInputsDesc() const
	{
		return { .Type	= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL,
				 .Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE |
						  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION,
				 .NumDescs		 = Size(),
				 .DescsLayout	 = D3D12_ELEMENTS_LAYOUT_ARRAY,
				 .pGeometryDescs = RaytracingGeometryDescs.data() };
	}

	void AddGeometry(const D3D12_RAYTRACING_GEOMETRY_DESC& Desc);

private:
	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> RaytracingGeometryDescs;
};

// https://developer.nvidia.com/blog/rtx-best-practices/
// We should rebuild the TLAS rather than update, It’s just easier to manage in most circumstances, and the cost savings
// to refit likely aren’t worth sacrificing quality of TLAS.
class D3D12RaytracingScene
{
public:
	D3D12RaytracingScene() noexcept = default;
	explicit D3D12RaytracingScene(size_t Size) { RaytracingInstanceDescs.reserve(Size); }

	auto begin() noexcept { return RaytracingInstanceDescs.begin(); }
	auto end() noexcept { return RaytracingInstanceDescs.end(); }

	[[nodiscard]] size_t size() const noexcept { return RaytracingInstanceDescs.size(); }
	[[nodiscard]] bool	 empty() const noexcept { return RaytracingInstanceDescs.empty(); }

	void Reset() noexcept;

	void AddInstance(const D3D12_RAYTRACING_INSTANCE_DESC& Desc);

	void ComputeMemoryRequirements(ID3D12Device5* Device, UINT64* pScratchSizeInBytes, UINT64* pResultSizeInBytes);

	void Generate(
		ID3D12GraphicsCommandList6* CommandList,
		ID3D12Resource*				Scratch,
		ID3D12Resource*				Result,
		D3D12_GPU_VIRTUAL_ADDRESS	InstanceDescs);

private:
	std::vector<D3D12_RAYTRACING_INSTANCE_DESC> RaytracingInstanceDescs;
	UINT64										ScratchSizeInBytes = 0;
	UINT64										ResultSizeInBytes  = 0;
};

// https://github.com/NVIDIAGameWorks/RTXMU, RTXMU is licensed under the MIT License.
// Modified for my own use

class D3D12RaytracingMemoryPage;

struct RaytracingMemorySection
{
	D3D12RaytracingMemoryPage* Parent		  = nullptr;
	UINT64					   Size			  = 0;
	UINT64					   Offset		  = 0;
	UINT64					   UnusedSize	  = 0;
	D3D12_GPU_VIRTUAL_ADDRESS  VirtualAddress = {};
	bool					   IsFree		  = false;
};

class D3D12RaytracingMemoryPage : public D3D12LinkedDeviceChild
{
public:
	D3D12RaytracingMemoryPage() noexcept = default;
	D3D12RaytracingMemoryPage(D3D12LinkedDevice* Parent, UINT64 PageSize)
		: D3D12LinkedDeviceChild(Parent)
		, PageSize(PageSize)
	{
	}

	void Initialize(D3D12_HEAP_TYPE HeapType, D3D12_RESOURCE_STATES InitialResourceState);

	UINT64 PageSize = 0;

	Microsoft::WRL::ComPtr<ID3D12Resource> Resource;
	D3D12_GPU_VIRTUAL_ADDRESS			   VirtualAddress = {};

	std::vector<RaytracingMemorySection> FreeMemorySections;
	UINT64								 CurrentOffset = 0;
	UINT64								 NumSubBlocks  = 0;
};

class D3D12RaytracingMemoryAllocator : public D3D12LinkedDeviceChild
{
public:
	D3D12RaytracingMemoryAllocator() noexcept = default;
	D3D12RaytracingMemoryAllocator(
		D3D12LinkedDevice*	  Parent,
		D3D12_HEAP_TYPE		  HeapType,
		D3D12_RESOURCE_STATES InitialResourceState,
		UINT64				  DefaultPageSize,
		UINT64				  Alignment)
		: D3D12LinkedDeviceChild(Parent)
		, HeapType(HeapType)
		, InitialResourceState(InitialResourceState)
		, DefaultPageSize(DefaultPageSize)
		, Alignment(Alignment)
	{
	}

	RaytracingMemorySection Allocate(UINT64 SizeInBytes)
	{
		// Align allocation
		const UINT64 AlignedSizeInBytes = AlignUp(SizeInBytes, Alignment);

		if (Pages.empty())
		{
			// Do not suballocate if the memory request is larger than the block size
			CreatePage(AlignedSizeInBytes > DefaultPageSize ? AlignedSizeInBytes : DefaultPageSize);
		}

		size_t NumPages = Pages.size();

		RaytracingMemorySection Section;

		if (AlignedSizeInBytes > DefaultPageSize)
		{
			CreatePage(AlignedSizeInBytes);

			D3D12RaytracingMemoryPage* Page = Pages.back().get();
			Section.Parent					= Page;
			Section.Size					= AlignedSizeInBytes;
			Section.Offset					= Page->CurrentOffset;
			Section.VirtualAddress			= Page->VirtualAddress + Page->CurrentOffset;

			const UINT64 OffsetInBytes = Page->CurrentOffset + AlignedSizeInBytes;
			Page->CurrentOffset		   = OffsetInBytes;
			Page->NumSubBlocks++;
		}
		else
		{
			for (size_t i = 0; i < NumPages; ++i)
			{
				D3D12RaytracingMemoryPage* Page = Pages[i].get();

				// Search within a block to find space for a new allocation
				// Modifies subBlock if able to suballocate in the block
				bool FoundFreeSubBlock	 = FindFreeSubBlock(Page, &Section, AlignedSizeInBytes);
				bool ContinueBlockSearch = false;

				// No memory reuse opportunities available so add a new suballocation
				if (!FoundFreeSubBlock)
				{
					UINT64 OffsetInBytes = Page->CurrentOffset + AlignedSizeInBytes;

					// Add a suballocation to the current offset of an existing block
					if (OffsetInBytes <= Page->PageSize)
					{
						// Only ever change the memory size if this is a new allocation
						Section.Size		   = AlignedSizeInBytes;
						Section.Offset		   = Page->CurrentOffset;
						Section.VirtualAddress = Page->VirtualAddress + Page->CurrentOffset;

						Page->CurrentOffset = OffsetInBytes;
						Page->NumSubBlocks++;
					}
					// If this block can't support this allocation
					else
					{
						// If all blocks traversed and suballocation doesn't fit then create a new block
						if (i == NumPages - 1)
						{
							// If suballocation block size is too small then do custom allocation of
							// individual blocks that match the resource's size
							UINT64 NewBlockSize =
								AlignedSizeInBytes > DefaultPageSize ? AlignedSizeInBytes : DefaultPageSize;
							CreatePage(NewBlockSize);
							NumPages++;
						}
						ContinueBlockSearch = true;
					}
				}
				// Assign SubBlock to the Block and discontinue suballocation search
				if (ContinueBlockSearch == false)
				{
					Section.Parent = Pages[i].get();
					break;
				}
			}
		}

		return Section;
	}

	void Release(RaytracingMemorySection* Section)
	{
		for (size_t i = 0; i < Pages.size(); ++i)
		{
			D3D12RaytracingMemoryPage* Page = Pages[i].get();
			if (Page == Section->Parent)
			{
				Section->IsFree = true;

				// Release the big chunks that are a single resource
				if (Section->Size == Page->PageSize)
				{
					Pages.erase(Pages.begin() + i);
				}
				else
				{
					Page->FreeMemorySections.push_back(*Section);

					Page->NumSubBlocks--;

					// If this suballocation was the final remaining allocation then release the suballocator block
					// but only if there is more than one block
					if ((Page->NumSubBlocks == 0) && (Pages.size() > 1))
					{
						Pages.erase(Pages.begin() + i);
					}
				}
				break;
			}
		}
	}

	UINT64 GetSize()
	{
		UINT64 Size = 0;
		for (const auto& Page : Pages)
		{
			Size += Page->PageSize;
		}
		return Size;
	}

	auto& GetPages() { return Pages; }

private:
	void CreatePage(UINT64 PageSize)
	{
		auto Page = std::make_unique<D3D12RaytracingMemoryPage>(GetParentLinkedDevice(), PageSize);
		Page->Initialize(HeapType, InitialResourceState);
		Pages.push_back(std::move(Page));
	}

	bool FindFreeSubBlock(
		D3D12RaytracingMemoryPage* Page,
		RaytracingMemorySection*   OutMemorySection,
		UINT64					   SizeInBytes)
	{
		bool foundFreeSubBlock	 = false;
		auto minUnusedMemoryIter = Page->FreeMemorySections.end();
		auto freeSubBlockIter	 = Page->FreeMemorySections.begin();

		uint64_t minUnusedMemorySubBlock = ~0ull;

		while (freeSubBlockIter != Page->FreeMemorySections.end())
		{
			if (SizeInBytes <= freeSubBlockIter->Size)
			{
				// Attempt to find the exact fit and if not fallback to the least wasted unused memory
				if (freeSubBlockIter->Size - SizeInBytes == 0)
				{
					// Keep previous allocation size
					OutMemorySection->Size	 = freeSubBlockIter->Size;
					OutMemorySection->Offset = freeSubBlockIter->Offset;
					foundFreeSubBlock		 = true;

					// Remove from the list
					Page->FreeMemorySections.erase(freeSubBlockIter);
					Page->NumSubBlocks++;
					break;
				}
				else
				{
					// Keep track of the available SubBlock with least fragmentation
					const uint64_t unusedMemory = freeSubBlockIter->Size - SizeInBytes;
					if (unusedMemory < minUnusedMemorySubBlock)
					{
						minUnusedMemoryIter		= freeSubBlockIter;
						minUnusedMemorySubBlock = unusedMemory;
					}
				}
			}
			++freeSubBlockIter;
		}

		// Did not find a perfect match so take the closest and get hit with fragmentation
		// Reject SubBlock if the closest available SubBlock is twice the required size
		if ((foundFreeSubBlock == false) && (minUnusedMemoryIter != Page->FreeMemorySections.end()) &&
			(minUnusedMemorySubBlock < 2 * SizeInBytes))
		{
			// Keep previous allocation size
			OutMemorySection->Size	 = minUnusedMemoryIter->Size;
			OutMemorySection->Offset = minUnusedMemoryIter->Offset;
			foundFreeSubBlock		 = true;

			// Remove from the list
			Page->FreeMemorySections.erase(minUnusedMemoryIter);
			++Page->NumSubBlocks;
		}

		return foundFreeSubBlock;
	}

	D3D12_HEAP_TYPE		  HeapType;
	D3D12_RESOURCE_STATES InitialResourceState;
	UINT64				  DefaultPageSize;
	UINT64				  Alignment;

	std::vector<std::unique_ptr<D3D12RaytracingMemoryPage>> Pages;
};

struct D3D12AccelerationStructure
{
	UINT64					CompactedSizeInBytes = 0;
	UINT64					ResultSize			 = 0;
	bool					IsCompacted			 = false;
	bool					RequestedCompaction	 = false;
	bool					ReadyToFree			 = false;
	RaytracingMemorySection ScratchMemory;
	RaytracingMemorySection ResultMemory;
	RaytracingMemorySection ResultCompactedMemory;
	RaytracingMemorySection CompactedSizeCpuMemory;
	RaytracingMemorySection CompactedSizeGpuMemory;

	D3D12SyncHandle SyncHandle;
};

class D3D12RaytracingAccelerationStructureManager : public D3D12LinkedDeviceChild
{
public:
	D3D12RaytracingAccelerationStructureManager() noexcept = default;
	D3D12RaytracingAccelerationStructureManager(D3D12LinkedDevice* Parent, UINT64 PageSize);

	UINT64 Build(
		ID3D12GraphicsCommandList4*									CommandList,
		const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& Inputs);

	void Copy(ID3D12GraphicsCommandList4* CommandList);

	// Returns true if compacted
	void Compact(ID3D12GraphicsCommandList4* CommandList, UINT64 AccelerationStructureIndex);

	void SetSyncHandle(UINT64 AccelerationStructureIndex, D3D12SyncHandle SyncHandle)
	{
		AccelerationStructures[AccelerationStructureIndex]->SyncHandle = SyncHandle;
	}

	D3D12_GPU_VIRTUAL_ADDRESS GetAccelerationStructureAddress(UINT64 AccelerationStructureIndex)
	{
		D3D12AccelerationStructure* AccelerationStructure = AccelerationStructures[AccelerationStructureIndex].get();

		return AccelerationStructure->IsCompacted ? AccelerationStructure->ResultCompactedMemory.VirtualAddress
												  : AccelerationStructure->ResultMemory.VirtualAddress;
	}

private:
	UINT64 GetAccelerationStructureIndex()
	{
		UINT64 NewIndex;
		if (!IndexQueue.empty())
		{
			NewIndex						 = IndexQueue.front();
			AccelerationStructures[NewIndex] = std::make_unique<D3D12AccelerationStructure>();
			IndexQueue.pop();
		}
		else
		{
			AccelerationStructures.push_back(std::make_unique<D3D12AccelerationStructure>());
			NewIndex = Index++;
		}
		return NewIndex;
	}

	void ReleaseAccelerationStructure(UINT64 Index)
	{
		IndexQueue.push(Index);
		AccelerationStructures[Index] = nullptr;
	}

private:
	UINT64 PageSize = 0;

	UINT64 TotalUncompactedMemory = 0;
	UINT64 TotalCompactedMemory	  = 0;

	std::vector<std::unique_ptr<D3D12AccelerationStructure>> AccelerationStructures;
	std::queue<UINT64>										 IndexQueue;
	UINT64													 Index = 0;

	D3D12RaytracingMemoryAllocator ScratchPool;
	D3D12RaytracingMemoryAllocator ResultPool;
	D3D12RaytracingMemoryAllocator ResultCompactedPool;
	D3D12RaytracingMemoryAllocator CompactedSizeGpuPool;
	D3D12RaytracingMemoryAllocator CompactedSizeCpuPool;
};

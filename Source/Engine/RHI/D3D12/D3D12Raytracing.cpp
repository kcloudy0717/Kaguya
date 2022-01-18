#include "D3D12Raytracing.h"

void D3D12RaytracingGeometry::AddGeometry(const D3D12_RAYTRACING_GEOMETRY_DESC& Desc)
{
	RaytracingGeometryDescs.push_back(Desc);
}

void D3D12RaytracingScene::Reset() noexcept
{
	RaytracingInstanceDescs.clear();
	ScratchSizeInBytes = 0;
	ResultSizeInBytes  = 0;
}

void D3D12RaytracingScene::AddInstance(const D3D12_RAYTRACING_INSTANCE_DESC& Desc)
{
	RaytracingInstanceDescs.push_back(Desc);
}

void D3D12RaytracingScene::ComputeMemoryRequirements(
	ID3D12Device5* Device,
	UINT64*		   pScratchSizeInBytes,
	UINT64*		   pResultSizeInBytes)
{
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS Inputs = {};
	Inputs.Type													= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	Inputs.Flags												= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
	Inputs.NumDescs												= static_cast<UINT>(RaytracingInstanceDescs.size());

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO PrebuildInfo = {};
	Device->GetRaytracingAccelerationStructurePrebuildInfo(&Inputs, &PrebuildInfo);

	ScratchSizeInBytes = D3D12RHIUtils::AlignUp<UINT64>(PrebuildInfo.ScratchDataSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
	ResultSizeInBytes  = D3D12RHIUtils::AlignUp<UINT64>(PrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);

	*pScratchSizeInBytes = ScratchSizeInBytes;
	*pResultSizeInBytes	 = ResultSizeInBytes;
}

void D3D12RaytracingScene::Generate(
	ID3D12GraphicsCommandList4* CommandList,
	ID3D12Resource*				Scratch,
	ID3D12Resource*				Result,
	D3D12_GPU_VIRTUAL_ADDRESS	InstanceDescs)
{
	assert(
		ScratchSizeInBytes > 0 && ResultSizeInBytes > 0 &&
		"Invalid allocation - ComputeMemoryRequirements needs to be called before Generate");
	assert(Result && Scratch && "Invalid Result, Scratch buffers");

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS Inputs = {};
	Inputs.Type													= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	Inputs.Flags												= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
	Inputs.NumDescs												= static_cast<UINT>(RaytracingInstanceDescs.size());
	Inputs.DescsLayout											= D3D12_ELEMENTS_LAYOUT_ARRAY;
	Inputs.InstanceDescs										= InstanceDescs;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC Desc = {};
	Desc.DestAccelerationStructureData						= Result->GetGPUVirtualAddress();
	Desc.Inputs												= Inputs;
	Desc.SourceAccelerationStructureData					= NULL;
	Desc.ScratchAccelerationStructureData					= Scratch->GetGPUVirtualAddress();

	// Build the top-level AS
	CommandList->BuildRaytracingAccelerationStructure(&Desc, 0, nullptr);
}

D3D12RaytracingMemoryPage::D3D12RaytracingMemoryPage(
	D3D12LinkedDevice*	  Parent,
	UINT64				  PageSize,
	D3D12_HEAP_TYPE		  HeapType,
	D3D12_RESOURCE_STATES InitialResourceState)
	: D3D12LinkedDeviceChild(Parent)
	, Resource(InitializeResource(PageSize, HeapType, InitialResourceState))
	, PageSize(PageSize)
	, VirtualAddress(Resource->GetGPUVirtualAddress())
{
}

ARC<ID3D12Resource> D3D12RaytracingMemoryPage::InitializeResource(UINT64 PageSize, D3D12_HEAP_TYPE HeapType, D3D12_RESOURCE_STATES InitialResourceState)
{
	ARC<ID3D12Resource>	  Resource;
	D3D12_HEAP_PROPERTIES HeapProperties = CD3DX12_HEAP_PROPERTIES(HeapType);
	D3D12_RESOURCE_DESC	  ResourceDesc	 = CD3DX12_RESOURCE_DESC::Buffer(PageSize);

	if (HeapType == D3D12_HEAP_TYPE_DEFAULT)
	{
		ResourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	VERIFY_D3D12_API(GetParentLinkedDevice()->GetDevice()->CreateCommittedResource(
		&HeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&ResourceDesc,
		InitialResourceState,
		nullptr,
		IID_PPV_ARGS(Resource.ReleaseAndGetAddressOf())));
	return Resource;
}

D3D12RaytracingMemoryAllocator::D3D12RaytracingMemoryAllocator(
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

RaytracingMemorySection D3D12RaytracingMemoryAllocator::Allocate(UINT64 SizeInBytes)
{
	// Align allocation
	const UINT64 AlignedSizeInBytes = D3D12RHIUtils::AlignUp(SizeInBytes, Alignment);

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

void D3D12RaytracingMemoryAllocator::Release(RaytracingMemorySection* Section)
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

UINT64 D3D12RaytracingMemoryAllocator::GetSize()
{
	UINT64 Size = 0;
	for (const auto& Page : Pages)
	{
		Size += Page->GetPageSize();
	}
	return Size;
}

void D3D12RaytracingMemoryAllocator::CreatePage(UINT64 PageSize)
{
	Pages.emplace_back(std::make_unique<D3D12RaytracingMemoryPage>(GetParentLinkedDevice(), PageSize, HeapType, InitialResourceState));
}

bool D3D12RaytracingMemoryAllocator::FindFreeSubBlock(
	D3D12RaytracingMemoryPage* Page,
	RaytracingMemorySection*   OutMemorySection,
	UINT64					   SizeInBytes)
{
	bool FoundFreeSubBlock	 = false;
	auto MinUnusedMemoryIter = Page->FreeMemorySections.end();
	auto FreeSubBlockIter	 = Page->FreeMemorySections.begin();

	uint64_t MinUnusedMemorySubBlock = ~0ull;

	while (FreeSubBlockIter != Page->FreeMemorySections.end())
	{
		if (SizeInBytes <= FreeSubBlockIter->Size)
		{
			// Attempt to find the exact fit and if not fallback to the least wasted unused memory
			if (FreeSubBlockIter->Size - SizeInBytes == 0)
			{
				// Keep previous allocation size
				OutMemorySection->Size	 = FreeSubBlockIter->Size;
				OutMemorySection->Offset = FreeSubBlockIter->Offset;
				FoundFreeSubBlock		 = true;

				// Remove from the list
				Page->FreeMemorySections.erase(FreeSubBlockIter);
				Page->NumSubBlocks++;
				break;
			}
			else
			{
				// Keep track of the available SubBlock with least fragmentation
				const uint64_t UnusedMemory = FreeSubBlockIter->Size - SizeInBytes;
				if (UnusedMemory < MinUnusedMemorySubBlock)
				{
					MinUnusedMemoryIter		= FreeSubBlockIter;
					MinUnusedMemorySubBlock = UnusedMemory;
				}
			}
		}
		++FreeSubBlockIter;
	}

	// Did not find a perfect match so take the closest and get hit with fragmentation
	// Reject SubBlock if the closest available SubBlock is twice the required size
	if ((FoundFreeSubBlock == false) && (MinUnusedMemoryIter != Page->FreeMemorySections.end()) &&
		(MinUnusedMemorySubBlock < 2 * SizeInBytes))
	{
		// Keep previous allocation size
		OutMemorySection->Size	 = MinUnusedMemoryIter->Size;
		OutMemorySection->Offset = MinUnusedMemoryIter->Offset;
		FoundFreeSubBlock		 = true;

		// Remove from the list
		Page->FreeMemorySections.erase(MinUnusedMemoryIter);
		++Page->NumSubBlocks;
	}

	return FoundFreeSubBlock;
}

D3D12RaytracingAccelerationStructureManager::D3D12RaytracingAccelerationStructureManager(
	D3D12LinkedDevice* Parent,
	UINT64			   PageSize)
	: D3D12LinkedDeviceChild(Parent)
	, PageSize(PageSize)
	, ScratchPool(
		  Parent,
		  D3D12_HEAP_TYPE_DEFAULT,
		  D3D12_RESOURCE_STATE_COMMON,
		  PageSize,
		  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT)
	, ResultPool(
		  Parent,
		  D3D12_HEAP_TYPE_DEFAULT,
		  D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		  PageSize,
		  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT)
	, ResultCompactedPool(
		  Parent,
		  D3D12_HEAP_TYPE_DEFAULT,
		  D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		  PageSize,
		  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT)
	, CompactedSizeGpuPool(
		  Parent,
		  D3D12_HEAP_TYPE_DEFAULT,
		  D3D12_RESOURCE_STATE_COMMON,
		  65536,
		  sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE_DESC))
	, CompactedSizeCpuPool(
		  Parent,
		  D3D12_HEAP_TYPE_READBACK,
		  D3D12_RESOURCE_STATE_COPY_DEST,
		  65536,
		  sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE_DESC))
{
}

UINT64 D3D12RaytracingAccelerationStructureManager::Build(
	ID3D12GraphicsCommandList4*									CommandList,
	const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& Inputs)
{
	UINT64 Index = GetAccelerationStructureIndex();

	// Request build size information and suballocate the scratch and result buffers
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO PrebuildInfo = {};
	GetParentLinkedDevice()->GetDevice5()->GetRaytracingAccelerationStructurePrebuildInfo(&Inputs, &PrebuildInfo);

	D3D12AccelerationStructure& AccelerationStructure = AccelerationStructures[Index];

	AccelerationStructure.ScratchMemory = ScratchPool.Allocate(PrebuildInfo.ScratchDataSizeInBytes);
	AccelerationStructure.ResultMemory	= ResultPool.Allocate(PrebuildInfo.ResultDataMaxSizeInBytes);

	TotalUncompactedMemory += AccelerationStructure.ResultMemory.Size;
	AccelerationStructure.ResultSize = AccelerationStructure.ResultMemory.Size;

	// Setup build desc and allocator scratch and result buffers
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC Desc = {};
	Desc.DestAccelerationStructureData						= AccelerationStructure.ResultMemory.VirtualAddress;
	Desc.Inputs												= Inputs;
	Desc.SourceAccelerationStructureData					= NULL;
	Desc.ScratchAccelerationStructureData					= AccelerationStructure.ScratchMemory.VirtualAddress;

	if (Inputs.Flags & D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION)
	{
		// Tag as not yet compacted
		AccelerationStructure.IsCompacted		  = false;
		AccelerationStructure.RequestedCompaction = true;

		// Suballocate the gpu memory that the builder will use to write the compaction size post build
		AccelerationStructure.CompactedSizeGpuMemory = CompactedSizeGpuPool.Allocate(
			sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE_DESC));
		// Suballocate the readback memory
		AccelerationStructure.CompactedSizeCpuMemory = CompactedSizeCpuPool.Allocate(
			sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE_DESC));

		// Request to get compaction size post build
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC PostbuildInfoDesc = {
			AccelerationStructure.CompactedSizeGpuMemory.VirtualAddress,
			D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE
		};

		CommandList->BuildRaytracingAccelerationStructure(&Desc, 1, &PostbuildInfoDesc);
	}
	else
	{
		AccelerationStructure.IsCompacted		  = false;
		AccelerationStructure.RequestedCompaction = false;
		CommandList->BuildRaytracingAccelerationStructure(&Desc, 0, nullptr);
	}

	return Index;
}

void D3D12RaytracingAccelerationStructureManager::Copy(
	ID3D12GraphicsCommandList4* CommandList)
{
	const auto& GpuPages = CompactedSizeGpuPool.GetPages();
	const auto& CpuPages = CompactedSizeCpuPool.GetPages();

	assert(GpuPages.size() < 32);
	D3D12_RESOURCE_BARRIER Barriers[32] = {};
	UINT				   NumBarriers	= 0;

	for (const auto& Page : GpuPages)
	{
		Barriers[NumBarriers++] = CD3DX12_RESOURCE_BARRIER::Transition(
			Page->GetResource(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_COPY_SOURCE);
	}
	CommandList->ResourceBarrier(NumBarriers, Barriers);

	for (size_t i = 0; i < GpuPages.size(); ++i)
	{
		CommandList->CopyResource(CpuPages[i]->GetResource(), GpuPages[i]->GetResource());
	}

	NumBarriers = 0;
	for (const auto& Page : GpuPages)
	{
		Barriers[NumBarriers++] = CD3DX12_RESOURCE_BARRIER::Transition(
			Page->GetResource(),
			D3D12_RESOURCE_STATE_COPY_SOURCE,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}
	CommandList->ResourceBarrier(NumBarriers, Barriers);
}

void D3D12RaytracingAccelerationStructureManager::Compact(
	ID3D12GraphicsCommandList4* CommandList,
	UINT64						AccelerationStructureIndex)
{
	D3D12AccelerationStructure& AccelerationStructure = AccelerationStructures[AccelerationStructureIndex];

	if (!AccelerationStructure.SyncHandle)
	{
		return;
	}

	// Readback data not available yet
	if (!AccelerationStructure.SyncHandle.IsComplete())
	{
		return;
	}

	// Only do compaction on the confirmed completion of the original build execution.
	if (AccelerationStructure.RequestedCompaction)
	{
		// Don't compact if not requested or already complete
		if (!AccelerationStructure.IsCompacted)
		{
			ID3D12Resource* CompactedResource = AccelerationStructure.CompactedSizeCpuMemory.Parent->GetResource();
			UINT64			Offset			  = AccelerationStructure.CompactedSizeCpuMemory.Offset;

			D3D12_RANGE Range = {};
			Range.Begin		  = Offset;
			Range.End		  = Offset + sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE_DESC);
			D3D12ScopedMap(
				CompactedResource,
				0,
				Range,
				[&](void* Data)
				{
					auto ByteData = static_cast<BYTE*>(Data);

					D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE_DESC Desc = {};
					memcpy(&Desc, &ByteData[Offset], sizeof(Desc));

					// Suballocate the gpu memory needed for compaction copy
					AccelerationStructure.ResultCompactedMemory =
						ResultCompactedPool.Allocate(Desc.CompactedSizeInBytes);

					AccelerationStructure.CompactedSizeInBytes = AccelerationStructure.ResultCompactedMemory.Size;
					TotalCompactedMemory += AccelerationStructure.ResultCompactedMemory.Size;

					// Copy the result buffer into the compacted buffer
					CommandList->CopyRaytracingAccelerationStructure(
						AccelerationStructure.ResultCompactedMemory.VirtualAddress,
						AccelerationStructure.ResultMemory.VirtualAddress,
						D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_COMPACT);

					// Tag as compaction complete
					AccelerationStructure.IsCompacted = true;
				});
		}
	}
}

void D3D12RaytracingAccelerationStructureManager::SetSyncHandle(
	UINT64			AccelerationStructureIndex,
	D3D12SyncHandle SyncHandle)
{
	AccelerationStructures[AccelerationStructureIndex].SyncHandle = SyncHandle;
}

D3D12_GPU_VIRTUAL_ADDRESS D3D12RaytracingAccelerationStructureManager::GetAccelerationStructureAddress(
	UINT64 AccelerationStructureIndex)
{
	const D3D12AccelerationStructure& AccelerationStructure = AccelerationStructures[AccelerationStructureIndex];

	return AccelerationStructure.IsCompacted ? AccelerationStructure.ResultCompactedMemory.VirtualAddress
											 : AccelerationStructure.ResultMemory.VirtualAddress;
}

UINT64 D3D12RaytracingAccelerationStructureManager::GetAccelerationStructureIndex()
{
	UINT64 NewIndex;
	if (!IndexQueue.empty())
	{
		NewIndex = IndexQueue.front();
		IndexQueue.pop();
	}
	else
	{
		AccelerationStructures.emplace_back();
		NewIndex = Index++;
	}
	return NewIndex;
}

void D3D12RaytracingAccelerationStructureManager::ReleaseAccelerationStructure(UINT64 Index)
{
	IndexQueue.push(Index);
	AccelerationStructures[Index] = {};
}

void D3D12RaytracingShaderBindingTable::Generate(D3D12LinkedDevice* Device)
{
	RayGenerationShaderTableOffset = SizeInBytes;
	SizeInBytes += RayGenerationShaderTable->GetTotalSizeInBytes();
	SizeInBytes = D3D12RHIUtils::AlignUp<UINT64>(SizeInBytes, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

	MissShaderTableOffset = SizeInBytes;
	SizeInBytes += MissShaderTable->GetTotalSizeInBytes();
	SizeInBytes = D3D12RHIUtils::AlignUp<UINT64>(SizeInBytes, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

	HitGroupShaderTableOffset = SizeInBytes;
	SizeInBytes += HitGroupShaderTable->GetTotalSizeInBytes();
	SizeInBytes = D3D12RHIUtils::AlignUp<UINT64>(SizeInBytes, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

	SBTBuffer		= D3D12Buffer(Device, SizeInBytes, 0, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_NONE);
	SBTUploadBuffer = D3D12Buffer(Device, SizeInBytes, 0, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_FLAG_NONE);
	SBTUploadBuffer.Initialize();
	CpuData = std::make_unique<BYTE[]>(SizeInBytes);
}

void D3D12RaytracingShaderBindingTable::WriteToGpu(ID3D12GraphicsCommandList* CommandList) const
{
	{
		BYTE* BaseAddress = CpuData.get();

		RayGenerationShaderTable->Write(BaseAddress + RayGenerationShaderTableOffset);
		MissShaderTable->Write(BaseAddress + MissShaderTableOffset);
		HitGroupShaderTable->Write(BaseAddress + HitGroupShaderTableOffset);
	}

	BYTE* Address = SBTUploadBuffer.GetCpuVirtualAddress<BYTE>();
	std::memcpy(Address, CpuData.get(), SizeInBytes);
	CommandList->CopyBufferRegion(SBTBuffer.GetResource(), 0, SBTUploadBuffer.GetResource(), 0, SizeInBytes);
}

D3D12_DISPATCH_RAYS_DESC D3D12RaytracingShaderBindingTable::GetDesc(
	UINT RayGenerationShaderIndex,
	UINT BaseMissShaderIndex) const
{
	UINT64 RayGenerationShaderTableSizeInBytes = RayGenerationShaderTable->GetSizeInBytes();
	UINT64 MissShaderTableSizeInBytes		   = MissShaderTable->GetSizeInBytes();
	UINT64 HitGroupShaderTableSizeInBytes	   = HitGroupShaderTable->GetSizeInBytes();

	UINT64 RayGenerationShaderRecordStride = RayGenerationShaderTable->GetStrideInBytes();
	UINT64 MissShaderRecordStride		   = MissShaderTable->GetStrideInBytes();
	UINT64 HitGroupShaderRecordStride	   = HitGroupShaderTable->GetStrideInBytes();

	D3D12_GPU_VIRTUAL_ADDRESS BaseAddress = SBTBuffer.GetGpuVirtualAddress();

	D3D12_DISPATCH_RAYS_DESC Desc  = {};
	Desc.RayGenerationShaderRecord = { .StartAddress = BaseAddress + RayGenerationShaderTableOffset + RayGenerationShaderIndex * RayGenerationShaderRecordStride,
									   .SizeInBytes	 = RayGenerationShaderTableSizeInBytes };
	Desc.MissShaderTable		   = { .StartAddress  = BaseAddress + MissShaderTableOffset + BaseMissShaderIndex * MissShaderRecordStride,
							   .SizeInBytes	  = MissShaderTableSizeInBytes,
							   .StrideInBytes = MissShaderRecordStride };
	Desc.HitGroupTable			   = { .StartAddress  = BaseAddress + HitGroupShaderTableOffset,
						   .SizeInBytes	  = HitGroupShaderTableSizeInBytes,
						   .StrideInBytes = HitGroupShaderRecordStride };
	return Desc;
}

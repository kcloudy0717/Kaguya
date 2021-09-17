#include "D3D12Raytracing.h"
#include "D3D12LinkedDevice.h"

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
	Inputs.Type		= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	Inputs.Flags	= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
	Inputs.NumDescs = static_cast<UINT>(RaytracingInstanceDescs.size());

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO PrebuildInfo = {};
	Device->GetRaytracingAccelerationStructurePrebuildInfo(&Inputs, &PrebuildInfo);

	ScratchSizeInBytes =
		AlignUp<UINT64>(PrebuildInfo.ScratchDataSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
	ResultSizeInBytes =
		AlignUp<UINT64>(PrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);

	*pScratchSizeInBytes = ScratchSizeInBytes;
	*pResultSizeInBytes	 = ResultSizeInBytes;
}

void D3D12RaytracingScene::Generate(
	ID3D12GraphicsCommandList6* CommandList,
	ID3D12Resource*				Scratch,
	ID3D12Resource*				Result,
	D3D12_GPU_VIRTUAL_ADDRESS	InstanceDescs)
{
	assert(
		ScratchSizeInBytes > 0 && ResultSizeInBytes > 0 &&
		"Invalid allocation - ComputeMemoryRequirements needs to be called before Generate");
	assert(Result && Scratch && "Invalid Result, Scratch buffers");

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS Inputs = {};
	Inputs.Type			 = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	Inputs.Flags		 = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
	Inputs.NumDescs		 = static_cast<UINT>(RaytracingInstanceDescs.size());
	Inputs.DescsLayout	 = D3D12_ELEMENTS_LAYOUT_ARRAY;
	Inputs.InstanceDescs = InstanceDescs;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC Desc = {};
	Desc.DestAccelerationStructureData						= Result->GetGPUVirtualAddress();
	Desc.Inputs												= Inputs;
	Desc.SourceAccelerationStructureData					= NULL;
	Desc.ScratchAccelerationStructureData					= Scratch->GetGPUVirtualAddress();

	// Build the top-level AS
	CommandList->BuildRaytracingAccelerationStructure(&Desc, 0, nullptr);
}

void D3D12RaytracingMemoryPage::Initialize(D3D12_HEAP_TYPE HeapType, D3D12_RESOURCE_STATES InitialResourceState)
{
	const D3D12_HEAP_PROPERTIES HeapProperties = CD3DX12_HEAP_PROPERTIES(HeapType);
	D3D12_RESOURCE_DESC			ResourceDesc   = CD3DX12_RESOURCE_DESC::Buffer(PageSize);

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

	VirtualAddress = Resource->GetGPUVirtualAddress();
}

D3D12RaytracingAccelerationStructureManager::D3D12RaytracingAccelerationStructureManager(
	D3D12LinkedDevice* Parent,
	UINT64			   PageSize)
	: D3D12LinkedDeviceChild(Parent)
	, PageSize(PageSize)
	, ScratchPool(
		  Parent,
		  D3D12_HEAP_TYPE_DEFAULT,
		  D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
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
		  D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
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

	D3D12AccelerationStructure* AccelerationStructure = AccelerationStructures[Index].get();

	AccelerationStructure->ScratchMemory = ScratchPool.Allocate(PrebuildInfo.ScratchDataSizeInBytes);
	AccelerationStructure->ResultMemory	 = ResultPool.Allocate(PrebuildInfo.ResultDataMaxSizeInBytes);

	TotalUncompactedMemory += AccelerationStructure->ResultMemory.Size;
	AccelerationStructure->ResultSize = AccelerationStructure->ResultMemory.Size;

	// Setup build desc and allocator scratch and result buffers
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC Desc = {};
	Desc.DestAccelerationStructureData						= AccelerationStructure->ResultMemory.VirtualAddress;
	Desc.Inputs												= Inputs;
	Desc.SourceAccelerationStructureData					= NULL;
	Desc.ScratchAccelerationStructureData					= AccelerationStructure->ScratchMemory.VirtualAddress;

	if (Inputs.Flags & D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION)
	{
		// Tag as not yet compacted
		AccelerationStructure->IsCompacted		   = false;
		AccelerationStructure->RequestedCompaction = true;

		// Suballocate the gpu memory that the builder will use to write the compaction size post build
		AccelerationStructure->CompactedSizeGpuMemory = CompactedSizeGpuPool.Allocate(
			sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE_DESC));
		// Suballocate the readback memory
		AccelerationStructure->CompactedSizeCpuMemory = CompactedSizeCpuPool.Allocate(
			sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE_DESC));

		// Request to get compaction size post build
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC PostbuildInfoDesc = {
			AccelerationStructure->CompactedSizeGpuMemory.VirtualAddress,
			D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE
		};

		CommandList->BuildRaytracingAccelerationStructure(&Desc, 1, &PostbuildInfoDesc);
	}
	else
	{
		AccelerationStructure->IsCompacted		   = false;
		AccelerationStructure->RequestedCompaction = false;
		CommandList->BuildRaytracingAccelerationStructure(&Desc, 0, nullptr);
	}

	return Index;
}

void D3D12RaytracingAccelerationStructureManager::Copy(ID3D12GraphicsCommandList4* CommandList)
{
	const auto& GpuPages = CompactedSizeGpuPool.GetPages();
	const auto& CpuPages = CompactedSizeCpuPool.GetPages();

	assert(GpuPages.size() < 32);
	D3D12_RESOURCE_BARRIER Barriers[32] = {};
	UINT				   NumBarriers	= 0;

	for (const auto& Page : GpuPages)
	{
		Barriers[NumBarriers++] = CD3DX12_RESOURCE_BARRIER::Transition(
			Page->Resource.Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_COPY_SOURCE);
	}
	CommandList->ResourceBarrier(NumBarriers, Barriers);

	for (size_t i = 0; i < GpuPages.size(); ++i)
	{
		CommandList->CopyResource(CpuPages[i]->Resource.Get(), GpuPages[i]->Resource.Get());
	}

	NumBarriers = 0;
	for (const auto& Page : GpuPages)
	{
		Barriers[NumBarriers++] = CD3DX12_RESOURCE_BARRIER::Transition(
			Page->Resource.Get(),
			D3D12_RESOURCE_STATE_COPY_SOURCE,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}
	CommandList->ResourceBarrier(NumBarriers, Barriers);
}

void D3D12RaytracingAccelerationStructureManager::Compact(
	ID3D12GraphicsCommandList4* CommandList,
	UINT64						AccelerationStructureIndex)
{
	D3D12AccelerationStructure* AccelerationStructure = AccelerationStructures[AccelerationStructureIndex].get();

	if (!AccelerationStructure->SyncPoint.IsValid())
	{
		return;
	}

	// Readback data not available yet
	if (!AccelerationStructure->SyncPoint.IsComplete())
	{
		return;
	}

	// Only do compaction on the confirmed completion of the original build execution.
	if (AccelerationStructure->RequestedCompaction)
	{
		// Don't compact if not requested or already complete
		if (AccelerationStructure->IsCompacted == false && AccelerationStructure->RequestedCompaction == true)
		{
			ID3D12Resource* CompactedResource = AccelerationStructure->CompactedSizeCpuMemory.Parent->Resource.Get();
			UINT64			Offset			  = AccelerationStructure->CompactedSizeCpuMemory.Offset;

			D3D12_RANGE Range = {};
			Range.Begin		  = Offset;
			Range.End = Offset + sizeof(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE_DESC);
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
					AccelerationStructure->ResultCompactedMemory =
						ResultCompactedPool.Allocate(Desc.CompactedSizeInBytes);

					AccelerationStructure->CompactedSizeInBytes = AccelerationStructure->ResultCompactedMemory.Size;
					TotalCompactedMemory += AccelerationStructure->ResultCompactedMemory.Size;

					// Copy the result buffer into the compacted buffer
					CommandList->CopyRaytracingAccelerationStructure(
						AccelerationStructure->ResultCompactedMemory.VirtualAddress,
						AccelerationStructure->ResultMemory.VirtualAddress,
						D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_COMPACT);

					// Tag as compaction complete
					AccelerationStructure->IsCompacted = true;
				});
		}
	}
}

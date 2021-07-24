#include "Raytracing.h"
#include "Device.h"

void BottomLevelAccelerationStructure::AddGeometry(const D3D12_RAYTRACING_GEOMETRY_DESC& Desc)
{
	RaytracingGeometryDescs.push_back(Desc);
}

void TopLevelAccelerationStructure::AddInstance(const D3D12_RAYTRACING_INSTANCE_DESC& Desc)
{
	RaytracingInstanceDescs.push_back(Desc);
}

void TopLevelAccelerationStructure::ComputeMemoryRequirements(
	ID3D12Device5* pDevice,
	UINT64*		   pScratchSizeInBytes,
	UINT64*		   pResultSizeInBytes)
{
	// Describe the work being requested, in this case the construction of a
	// (possibly dynamic) top-level hierarchy, with the given instance descriptors
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS Inputs = {
		.Type			= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL,
		.Flags			= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD,
		.NumDescs		= static_cast<UINT>(RaytracingInstanceDescs.size()),
		.DescsLayout	= D3D12_ELEMENTS_LAYOUT_ARRAY,
		.pGeometryDescs = nullptr
	};

	// This structure is used to hold the sizes of the required scratch memory and
	// resulting AS
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO PrebuildInfo = {};

	// Building the acceleration structure (AS) requires some scratch space, as
	// well as space to store the resulting structure This function computes a
	// conservative estimate of the memory requirements for both, based on the
	// number of bottom-level instances.
	pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&Inputs, &PrebuildInfo);

	ScratchSizeInBytes =
		AlignUp<UINT64>(PrebuildInfo.ScratchDataSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
	ResultSizeInBytes =
		AlignUp<UINT64>(PrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);

	*pScratchSizeInBytes = ScratchSizeInBytes;
	*pResultSizeInBytes	 = ResultSizeInBytes;
}

void TopLevelAccelerationStructure::Generate(
	ID3D12GraphicsCommandList6* pCommandList,
	ID3D12Resource*				pScratch,
	ID3D12Resource*				pResult,
	D3D12_GPU_VIRTUAL_ADDRESS	InstanceDescs)
{
	if (ScratchSizeInBytes == 0 || ResultSizeInBytes == 0)
	{
		throw std::logic_error("Invalid allocation - ComputeMemoryRequirements needs to be called before Generate");
	}

	if (!pResult || !pScratch)
	{
		throw std::logic_error("Invalid pResult, pScratch buffers");
	}

	const D3D12_GPU_VIRTUAL_ADDRESS Dest	= pResult->GetGPUVirtualAddress();
	const D3D12_GPU_VIRTUAL_ADDRESS Scratch = pScratch->GetGPUVirtualAddress();

	// Create a descriptor of the requested builder work, to generate a top - level
	// AS from the input parameters
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS Inputs = {
		.Type		   = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL,
		.Flags		   = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD,
		.NumDescs	   = static_cast<UINT>(size()),
		.DescsLayout   = D3D12_ELEMENTS_LAYOUT_ARRAY,
		.InstanceDescs = InstanceDescs
	};

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC Desc = { .DestAccelerationStructureData	  = Dest,
																.Inputs							  = Inputs,
																.SourceAccelerationStructureData  = NULL,
																.ScratchAccelerationStructureData = Scratch };

	// Build the top-level AS
	pCommandList->BuildRaytracingAccelerationStructure(&Desc, 0, nullptr);
}

void RaytracingMemoryPage::Initialize(D3D12_HEAP_TYPE HeapType, D3D12_RESOURCE_STATES InitialResourceState)
{
	const D3D12_HEAP_PROPERTIES HeapProperties = CD3DX12_HEAP_PROPERTIES(HeapType);
	D3D12_RESOURCE_DESC			ResourceDesc   = CD3DX12_RESOURCE_DESC::Buffer(PageSize);

	if (HeapType == D3D12_HEAP_TYPE_DEFAULT)
	{
		ResourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	ASSERTD3D12APISUCCEEDED(GetParentDevice()->GetDevice()->CreateCommittedResource(
		&HeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&ResourceDesc,
		InitialResourceState,
		nullptr,
		IID_PPV_ARGS(pResource.ReleaseAndGetAddressOf())));

	GPUVirtualAddress = pResource->GetGPUVirtualAddress();
}

UINT64 RaytracingAccelerationStructureManager::Build(
	ID3D12GraphicsCommandList4*									CommandList,
	const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& Inputs)
{
	UINT64 Index = GetAccelerationStructureIndex();

	// Request build size information and suballocate the scratch and result buffers
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO PrebuildInfo = {};
	GetParentDevice()->GetDevice5()->GetRaytracingAccelerationStructurePrebuildInfo(&Inputs, &PrebuildInfo);

	AccelerationStructure* AccelerationStructure = AccelerationStructures[Index].get();

	AccelerationStructure->ScratchMemory = ScratchPool.Allocate(PrebuildInfo.ScratchDataSizeInBytes);
	AccelerationStructure->ResultMemory	 = ResultPool.Allocate(PrebuildInfo.ResultDataMaxSizeInBytes);

	TotalUncompactedMemory += AccelerationStructure->ResultMemory.Size;
	AccelerationStructure->ResultSize = AccelerationStructure->ResultMemory.Size;

	// Setup build desc and allocator scratch and result buffers
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC Desc = {
		.DestAccelerationStructureData	  = AccelerationStructure->ResultMemory.GPUVirtualAddress,
		.Inputs							  = Inputs,
		.SourceAccelerationStructureData  = NULL,
		.ScratchAccelerationStructureData = AccelerationStructure->ScratchMemory.GPUVirtualAddress
	};

	if (Inputs.Flags & D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION)
	{
		// Tag as not yet compacted
		AccelerationStructure->IsCompacted		   = false;
		AccelerationStructure->RequestedCompaction = true;

		// Suballocate the gpu memory that the builder will use to write the compaction size post build
		AccelerationStructure->CompactedSizeGpuMemory =
			CompactedSizeCPUPool.Allocate(SizeOfPostBuildInfoCompactedSizeDesc);
		// Suballocate the readback memory
		AccelerationStructure->CompactedSizeCpuMemory =
			CompactedSizeGPUPool.Allocate(SizeOfPostBuildInfoCompactedSizeDesc);

		// Request to get compaction size post build
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC PostbuildInfoDesc = {
			AccelerationStructure->CompactedSizeGpuMemory.GPUVirtualAddress,
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

void RaytracingAccelerationStructureManager::Copy(ID3D12GraphicsCommandList4* CommandList)
{
	const auto& GPUPages = CompactedSizeCPUPool.GetPages();
	const auto& CPUPages = CompactedSizeGPUPool.GetPages();

	assert(GPUPages.size() < 32);

	UINT				   NumBarriers = 0;
	D3D12_RESOURCE_BARRIER Barriers[32];

	for (const auto& i : GPUPages)
	{
		ID3D12Resource* GPUPage = i->pResource.Get();

		// Transition the gpu compaction size suballocator block to copy over to mappable cpu memory
		Barriers[NumBarriers++] = CD3DX12_RESOURCE_BARRIER::Transition(
			GPUPage,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_COPY_SOURCE);
	}
	CommandList->ResourceBarrier(NumBarriers, Barriers);

	for (size_t i = 0; i < GPUPages.size(); ++i)
	{
		ID3D12Resource* CPUPage = CPUPages[i]->pResource.Get();
		ID3D12Resource* GPUPage = GPUPages[i]->pResource.Get();

		// Copy the entire resource to avoid individually copying over each compaction size in strides of 8 bytes
		CommandList->CopyResource(CPUPage, GPUPage);
	}

	NumBarriers = 0;
	for (const auto& i : GPUPages)
	{
		ID3D12Resource* GPUPage = i->pResource.Get();

		Barriers[NumBarriers++] = CD3DX12_RESOURCE_BARRIER::Transition(
			GPUPage,
			D3D12_RESOURCE_STATE_COPY_SOURCE,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}
	CommandList->ResourceBarrier(NumBarriers, Barriers);
}

void RaytracingAccelerationStructureManager::Compact(
	ID3D12GraphicsCommandList4* CommandList,
	UINT64						AccelerationStructureIndex)
{
	AccelerationStructure* AccelerationStructure = AccelerationStructures[AccelerationStructureIndex].get();

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
		if ((AccelerationStructure->IsCompacted == false) && (AccelerationStructure->RequestedCompaction == true))
		{
			D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE_DESC Desc = {};
			UINT64 Offset = AccelerationStructure->CompactedSizeCpuMemory.Offset;

			// Map the readback gpu memory to system memory to fetch compaction size
			D3D12_RANGE Range = { Offset, Offset + SizeOfPostBuildInfoCompactedSizeDesc };
			BYTE*		pData = nullptr;
			if (SUCCEEDED(
					AccelerationStructure->CompactedSizeCpuMemory.Parent->pResource->Map(0, &Range, (void**)&pData)))
			{
				memcpy(&Desc, &pData[Offset], sizeof(Desc));
				AccelerationStructure->CompactedSizeCpuMemory.Parent->pResource->Unmap(0, &Range);

				// Suballocate the gpu memory needed for compaction copy
				AccelerationStructure->ResultCompactedMemory = ResultCompactedPool.Allocate(Desc.CompactedSizeInBytes);

				AccelerationStructure->CompactedSizeInBytes = AccelerationStructure->ResultCompactedMemory.Size;
				TotalCompactedMemory += AccelerationStructure->ResultCompactedMemory.Size;

				// Copy the result buffer into the compacted buffer
				CommandList->CopyRaytracingAccelerationStructure(
					AccelerationStructure->ResultCompactedMemory.GPUVirtualAddress,
					AccelerationStructure->ResultMemory.GPUVirtualAddress,
					D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_COMPACT);

				// Tag as compaction complete
				AccelerationStructure->IsCompacted = true;
			}
		}
	}
}

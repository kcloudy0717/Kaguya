#include "pch.h"
#include "AccelerationStructure.h"

BottomLevelAccelerationStructure::BottomLevelAccelerationStructure() noexcept
	: m_ScratchSizeInBytes(0)
	, m_ResultSizeInBytes(0)
{
}

void BottomLevelAccelerationStructure::AddGeometry(_In_ const D3D12_RAYTRACING_GEOMETRY_DESC& Desc)
{
	m_RaytracingGeometryDescs.push_back(Desc);
}

void BottomLevelAccelerationStructure::ComputeMemoryRequirements(
	_In_ ID3D12Device5* pDevice,
	_Out_ UINT64* pScratchSizeInBytes,
	_Out_ UINT64* pResultSizeInBytes)
{
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS desc = {};
	desc.Type			= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	desc.Flags			= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	desc.NumDescs		= static_cast<UINT>(m_RaytracingGeometryDescs.size());
	desc.DescsLayout	= D3D12_ELEMENTS_LAYOUT_ARRAY;
	desc.pGeometryDescs = m_RaytracingGeometryDescs.data();

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
	pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&desc, &prebuildInfo);

	// Buffer sizes need to be 256-byte-aligned
	m_ScratchSizeInBytes =
		AlignUp<UINT64>(prebuildInfo.ScratchDataSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
	m_ResultSizeInBytes =
		AlignUp<UINT64>(prebuildInfo.ResultDataMaxSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);

	*pScratchSizeInBytes = m_ScratchSizeInBytes;
	*pResultSizeInBytes	 = m_ResultSizeInBytes;
}

void BottomLevelAccelerationStructure::Generate(
	_In_ ID3D12GraphicsCommandList6* pCommandList,
	_In_ ID3D12Resource* pScratch,
	_In_ ID3D12Resource* pResult)
{
	if (m_ScratchSizeInBytes == 0 || m_ResultSizeInBytes == 0)
	{
		throw std::logic_error(
			"Invalid Result and Scratch buffer sizes - ComputeMemoryRequirements needs to be called before Build");
	}

	if (!pResult || !pScratch)
	{
		throw std::logic_error("Invalid pResult and pScratch buffers");
	}

	// Create a descriptor of the requested builder work, to generate a
	// bottom-level AS from the input parameters
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc = {};
	desc.DestAccelerationStructureData						= pResult->GetGPUVirtualAddress();
	desc.Inputs.Type										= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	desc.Inputs.Flags					  = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	desc.Inputs.NumDescs				  = static_cast<UINT>(m_RaytracingGeometryDescs.size());
	desc.Inputs.DescsLayout				  = D3D12_ELEMENTS_LAYOUT_ARRAY;
	desc.Inputs.pGeometryDescs			  = m_RaytracingGeometryDescs.data();
	desc.SourceAccelerationStructureData  = NULL;
	desc.ScratchAccelerationStructureData = pScratch->GetGPUVirtualAddress();

	// Build the AS
	pCommandList->BuildRaytracingAccelerationStructure(&desc, 0, nullptr);
}

TopLevelAccelerationStructure::TopLevelAccelerationStructure()
	: m_ScratchSizeInBytes(0)
	, m_ResultSizeInBytes(0)
{
}

void TopLevelAccelerationStructure::AddInstance(_In_ const D3D12_RAYTRACING_INSTANCE_DESC& Desc)
{
	m_RaytracingInstanceDescs.push_back(Desc);
}

void TopLevelAccelerationStructure::ComputeMemoryRequirements(
	_In_ ID3D12Device5* pDevice,
	_Out_ UINT64* pScratchSizeInBytes,
	_Out_ UINT64* pResultSizeInBytes)
{
	// Describe the work being requested, in this case the construction of a
	// (possibly dynamic) top-level hierarchy, with the given instance descriptors
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS desc = {};
	desc.Type												  = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	desc.Flags		 = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
	desc.NumDescs	 = size();
	desc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

	// This structure is used to hold the sizes of the required scratch memory and
	// resulting AS
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};

	// Building the acceleration structure (AS) requires some scratch space, as
	// well as space to store the resulting structure This function computes a
	// conservative estimate of the memory requirements for both, based on the
	// number of bottom-level instances.
	pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&desc, &prebuildInfo);

	// Buffer sizes need to be 256-byte-aligned
	m_ScratchSizeInBytes =
		AlignUp<UINT64>(prebuildInfo.ScratchDataSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
	m_ResultSizeInBytes =
		AlignUp<UINT64>(prebuildInfo.ResultDataMaxSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);

	*pScratchSizeInBytes = m_ScratchSizeInBytes;
	*pResultSizeInBytes	 = m_ResultSizeInBytes;
}

void TopLevelAccelerationStructure::Generate(
	_In_ ID3D12GraphicsCommandList6* pCommandList,
	_In_ ID3D12Resource* pScratch,
	_In_ ID3D12Resource*		   pResult,
	_In_ D3D12_GPU_VIRTUAL_ADDRESS InstanceDescs)
{
	if (m_ScratchSizeInBytes == 0 || m_ResultSizeInBytes == 0)
	{
		throw std::logic_error("Invalid allocation - ComputeMemoryRequirements needs to be called before Generate");
	}

	if (!pResult || !pScratch)
	{
		throw std::logic_error("Invalid pResult, pScratch buffers");
	}

	// Create a descriptor of the requested builder work, to generate a top - level
	// AS from the input parameters
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc = {};
	desc.DestAccelerationStructureData						= pResult->GetGPUVirtualAddress();
	desc.Inputs.Type										= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	desc.Inputs.Flags					  = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
	desc.Inputs.NumDescs				  = size();
	desc.Inputs.DescsLayout				  = D3D12_ELEMENTS_LAYOUT_ARRAY;
	desc.Inputs.InstanceDescs			  = InstanceDescs;
	desc.SourceAccelerationStructureData  = NULL;
	desc.ScratchAccelerationStructureData = pScratch->GetGPUVirtualAddress();

	// Build the top-level AS
	pCommandList->BuildRaytracingAccelerationStructure(&desc, 0, nullptr);
}

#pragma once
#include <World/Vertex.h>
#include "D3D12/D3D12Device.h"

namespace Asset
{
struct MeshMetadata
{
	std::filesystem::path Path;
	bool				  KeepGeometryInRAM;
};

struct Submesh
{
	uint32_t IndexCount;
	uint32_t StartIndexLocation;
	uint32_t VertexCount;
	uint32_t BaseVertexLocation;
};

struct Mesh
{
	MeshMetadata Metadata;

	std::string Name;

	std::vector<Vertex>	  Vertices;
	std::vector<uint32_t> Indices;

	std::vector<Submesh> Submeshes;

	D3D12Buffer				  VertexResource;
	D3D12Buffer				  IndexResource;
	D3D12_GPU_VIRTUAL_ADDRESS AccelerationStructure;
	D3D12RaytracingGeometry	  BLAS;
	UINT64					  BLASIndex		= UINT64_MAX;
	bool					  BLASValid		= false;
	bool					  BLASCompacted = false;
};
} // namespace Asset

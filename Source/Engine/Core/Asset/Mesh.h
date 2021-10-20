#pragma once
#include "Asset.h"
#include <World/Vertex.h>
#include "Core/RHI/D3D12/D3D12Raytracing.h"

struct MeshImportOptions
{
	MeshImportOptions() { XMStoreFloat4x4(&Matrix, DirectX::XMMatrixIdentity()); }

	std::filesystem::path Path;

	Vector3f			Translation	 = { 0.0f, 0.0f, 0.0f };
	Vector3f			Rotation	 = { 0.0f, 0.0f, 0.0f };
	float				UniformScale = 1.0f;
	DirectX::XMFLOAT4X4 Matrix;
};

class Mesh : public Asset
{
public:
	void ComputeBoundingBox()
	{
		DirectX::BoundingBox Box;
		DirectX::BoundingBox::CreateFromPoints(Box, Vertices.size(), &Vertices[0].Position, sizeof(Vertex));
		BoundingBox.Center	= Vector3f(Box.Center.x, Box.Center.y, Box.Center.z);
		BoundingBox.Extents = Vector3f(Box.Extents.x, Box.Extents.y, Box.Extents.z);
	}

	MeshImportOptions Options;

	std::string Name;

	std::vector<Vertex>					  Vertices;
	std::vector<uint32_t>				  Indices;
	std::vector<DirectX::Meshlet>		  Meshlets;
	std::vector<uint8_t>				  UniqueVertexIndices;
	std::vector<DirectX::MeshletTriangle> PrimitiveIndices;

	BoundingBox BoundingBox;

	D3D12Buffer				  VertexResource;
	D3D12Buffer				  IndexResource;
	D3D12Buffer				  MeshletResource;
	D3D12Buffer				  UniqueVertexIndexResource;
	D3D12Buffer				  PrimitiveIndexResource;
	D3D12_GPU_VIRTUAL_ADDRESS AccelerationStructure; // Managed by D3D12RaytracingAccelerationStructureManager
	D3D12RaytracingGeometry	  Blas;
	UINT64					  BlasIndex		= UINT64_MAX;
	bool					  BlasValid		= false;
	bool					  BlasCompacted = false;
};

template<>
struct AssetTypeTraits<AssetType::Mesh>
{
	using Type = Mesh;
};

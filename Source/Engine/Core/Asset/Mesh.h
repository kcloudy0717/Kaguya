#pragma once
#include "Asset.h"
#include "World/Vertex.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/BoundingBox.h"
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

	void UpdateInfo()
	{
		VertexCount		 = static_cast<std::uint32_t>(Vertices.size());
		IndexCount		 = static_cast<std::uint32_t>(Indices.size());
		MeshletCount	 = static_cast<std::uint32_t>(Meshlets.size());
		VertexIndexCount = static_cast<std::uint32_t>(UniqueVertexIndices.size());
		PrimitiveCount	 = static_cast<std::uint32_t>(PrimitiveIndices.size());
	}

	void Release()
	{
		// Physics needs this
		// std::vector<Vertex>().swap(Vertices);
		// std::vector<uint32_t>().swap(Indices);
		std::vector<DirectX::Meshlet>().swap(Meshlets);
		std::vector<uint8_t>().swap(UniqueVertexIndices);
		std::vector<DirectX::MeshletTriangle>().swap(PrimitiveIndices);
	}

	MeshImportOptions Options;

	std::string Name;

	std::uint32_t VertexCount	   = 0;
	std::uint32_t IndexCount	   = 0;
	std::uint32_t MeshletCount	   = 0;
	std::uint32_t VertexIndexCount = 0;
	std::uint32_t PrimitiveCount   = 0;

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

	D3D12ShaderResourceView VertexView;
	D3D12ShaderResourceView IndexView;
};

template<>
struct AssetTypeTraits<AssetType::Mesh>
{
	using Type = Mesh;
};

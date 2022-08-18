#pragma once
#include "IAsset.h"
#include "Core/World/Vertex.h"
#include "RHI/RHI.h"
#include "Math/Math.h"
#include <DirectXMesh.h>

namespace Asset
{
	struct MeshImportOptions
	{
		MeshImportOptions()
		{
			XMStoreFloat4x4(&Matrix, DirectX::XMMatrixIdentity());
		}

		std::filesystem::path Path;

		bool GenerateMeshlets = false;

		Math::Vec3f			Translation	 = { 0.0f, 0.0f, 0.0f };
		Math::Vec3f			Rotation	 = { 0.0f, 0.0f, 0.0f };
		float				UniformScale = 1.0f;
		DirectX::XMFLOAT4X4 Matrix;
	};

	class Mesh : public IAsset
	{
	public:
		void ComputeBoundingBox();

		void UpdateInfo();

		void Release();

		void SetVertices(std::vector<Vertex>&& Vertices);
		void SetIndices(std::vector<u32>&& Indices);
		void SetMeshlets(std::vector<DirectX::Meshlet>&& Meshlets);
		void SetUniqueVertexIndices(std::vector<u8>&& UniqueVertexIndices);
		void SetPrimitiveIndices(std::vector<DirectX::MeshletTriangle>&& PrimitiveIndices);

		MeshImportOptions Options;

		std::string Name;

		u32 NumVertices		 = 0;
		u32 NumIndices		 = 0;
		u32 NumMeshlets		 = 0;
		u32 NumVertexIndices = 0;
		u32 NumPrimitives	 = 0;

		std::vector<Vertex>					  Vertices;
		std::vector<u32>					  Indices;
		std::vector<DirectX::Meshlet>		  Meshlets;
		std::vector<u8>						  UniqueVertexIndices;
		std::vector<DirectX::MeshletTriangle> PrimitiveIndices;

		Math::BoundingBox BoundingBox;

		RHI::D3D12Buffer			 VertexResource;
		RHI::D3D12Buffer			 IndexResource;
		RHI::D3D12Buffer			 MeshletResource;
		RHI::D3D12Buffer			 UniqueVertexIndexResource;
		RHI::D3D12Buffer			 PrimitiveIndexResource;
		RHI::D3D12RaytracingGeometry Blas;

		RHI::D3D12ShaderResourceView VertexView;
		RHI::D3D12ShaderResourceView IndexView;
	};

	template<>
	struct AssetTypeTraits<Mesh>
	{
		static constexpr AssetType Enum = AssetType::Mesh;
		using ApiType					= Mesh;
	};
} // namespace Asset

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

		void ResetRaytracingInfo();

		MeshImportOptions Options;

		std::string Name;

		u32 VertexCount		 = 0;
		u32 IndexCount		 = 0;
		u32 MeshletCount	 = 0;
		u32 VertexIndexCount = 0;
		u32 PrimitiveCount	 = 0;

		std::vector<Vertex>					  Vertices;
		std::vector<uint32_t>				  Indices;
		std::vector<DirectX::Meshlet>		  Meshlets;
		std::vector<uint8_t>				  UniqueVertexIndices;
		std::vector<DirectX::MeshletTriangle> PrimitiveIndices;

		Math::BoundingBox BoundingBox;

		RHI::D3D12Buffer			 VertexResource;
		RHI::D3D12Buffer			 IndexResource;
		RHI::D3D12Buffer			 MeshletResource;
		RHI::D3D12Buffer			 UniqueVertexIndexResource;
		RHI::D3D12Buffer			 PrimitiveIndexResource;
		RHI::D3D12RaytracingGeometry Blas;
		D3D12_GPU_VIRTUAL_ADDRESS	 AccelerationStructure = {}; // Managed by D3D12RaytracingManager
		u64							 BlasIndex			   = UINT64_MAX;
		bool						 BlasValid			   = false;
		bool						 BlasCompacted		   = false;

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

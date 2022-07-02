#pragma once
#include <RHI/RHI.h>
#include <Core/World/World.h>
#include "RaytracingAccelerationStructure.h"

namespace Hlsl
{
	struct Material
	{
		unsigned int	  BSDFType;
		DirectX::XMFLOAT3 BaseColor;
		float			  Metallic;
		float			  Subsurface;
		float			  Specular;
		float			  Roughness;
		float			  SpecularTint;
		float			  Anisotropic;
		float			  Sheen;
		float			  SheenTint;
		float			  Clearcoat;
		float			  ClearcoatGloss;

		// Used by Glass BxDF
		DirectX::XMFLOAT3 T;
		float			  EtaA, EtaB;

		int Albedo;
	};

	struct Light
	{
		unsigned int	  Type;
		DirectX::XMFLOAT3 Position;
		DirectX::XMFLOAT4 Orientation;
		float			  Width;
		float			  Height;
		DirectX::XMFLOAT3 Points[4]; // World-space points for quad light type

		DirectX::XMFLOAT3 I;
		float			  Intensity;
		float			  Radius;
		float			  InnerAngle;
		float			  OuterAngle;
	};

	struct Mesh
	{
		// 64
		DirectX::XMFLOAT4X4 Transform;
		// 64
		DirectX::XMFLOAT4X4 PreviousTransform;

		// 64
		D3D12_VERTEX_BUFFER_VIEW  VertexBuffer;
		D3D12_INDEX_BUFFER_VIEW	  IndexBuffer;
		D3D12_GPU_VIRTUAL_ADDRESS Meshlets;
		D3D12_GPU_VIRTUAL_ADDRESS UniqueVertexIndices;
		D3D12_GPU_VIRTUAL_ADDRESS PrimitiveIndices;
		D3D12_GPU_VIRTUAL_ADDRESS AccelerationStructure;

		// 24
		Math::BoundingBox BoundingBox;
		// 20
		D3D12_DRAW_INDEXED_ARGUMENTS DrawIndexedArguments;

		// 20
		unsigned int MaterialIndex;
		unsigned int NumMeshlets;
		unsigned int VertexView;
		unsigned int IndexView;
		unsigned int DEADBEEF2 = 0xDEADBEEF;
	};
	static_assert(sizeof(Mesh) == 256);

	struct Camera
	{
		float FoVY; // Degrees
		float AspectRatio;
		float NearZ;
		float FarZ;

		float		 FocalLength;
		float		 RelativeAperture;
		unsigned int DEADBEEF0 = 0xDEADBEEF;
		unsigned int DEADBEEF1 = 0xDEADBEEF;

		DirectX::XMFLOAT4 Position;

		DirectX::XMFLOAT4X4 View;
		DirectX::XMFLOAT4X4 Projection;
		DirectX::XMFLOAT4X4 ViewProjection;

		DirectX::XMFLOAT4X4 InvView;
		DirectX::XMFLOAT4X4 InvProjection;
		DirectX::XMFLOAT4X4 InvViewProjection;

		DirectX::XMFLOAT4X4 PrevViewProjection;

		Math::Frustum Frustum;
	};
} // namespace Hlsl

inline Hlsl::Material GetHLSLMaterialDesc(const Material& Material)
{
	return { .BSDFType		 = (unsigned int)Material.BSDFType,
			 .BaseColor		 = Material.BaseColor,
			 .Metallic		 = Material.Metallic,
			 .Subsurface	 = Material.Subsurface,
			 .Specular		 = Material.Specular,
			 .Roughness		 = Material.Roughness,
			 .SpecularTint	 = Material.SpecularTint,
			 .Anisotropic	 = Material.Anisotropic,
			 .Sheen			 = Material.Sheen,
			 .SheenTint		 = Material.SheenTint,
			 .Clearcoat		 = Material.Clearcoat,
			 .ClearcoatGloss = Material.ClearcoatGloss,

			 .T	   = Material.T,
			 .EtaA = Material.EtaA,
			 .EtaB = Material.EtaB,

			 .Albedo = Material.TextureIndices[0] };
}

inline Hlsl::Light GetHLSLLightDesc(const Math::Transform& Transform, const LightComponent& Light)
{
	using namespace DirectX;

	XMMATRIX M = Transform.Matrix();

	XMFLOAT4 Orientation;
	XMStoreFloat4(&Orientation, DirectX::XMVector3Normalize(Transform.Forward()));
	float HalfWidth	 = Light.Width * 0.5f;
	float HalfHeight = Light.Height * 0.5f;
	// Get local space point
	XMVECTOR P0 = XMVectorSet(+HalfWidth, -HalfHeight, 0, 1);
	XMVECTOR P1 = XMVectorSet(+HalfWidth, +HalfHeight, 0, 1);
	XMVECTOR P2 = XMVectorSet(-HalfWidth, +HalfHeight, 0, 1);
	XMVECTOR P3 = XMVectorSet(-HalfWidth, -HalfHeight, 0, 1);

	// Precompute the light points here so ray generation shader doesnt have to do it for every ray
	// Move points to light's location
	XMFLOAT3 Points[4] = {};
	XMStoreFloat3(&Points[0], XMVector3TransformCoord(P0, M));
	XMStoreFloat3(&Points[1], XMVector3TransformCoord(P1, M));
	XMStoreFloat3(&Points[2], XMVector3TransformCoord(P2, M));
	XMStoreFloat3(&Points[3], XMVector3TransformCoord(P3, M));

	return {
		.Type		 = (unsigned int)Light.Type,
		.Position	 = Transform.Position,
		.Orientation = Orientation,
		.Width		 = Light.Width,
		.Height		 = Light.Height,
		.Points		 = { Points[0], Points[1], Points[2], Points[3] },

		.I			= Light.I,
		.Intensity	= 1.0f,
		.Radius		= Light.Radius,
		.InnerAngle = Light.InnerAngle,
		.OuterAngle = Light.OuterAngle,
	};
}

inline Hlsl::Mesh GetHLSLMeshDesc(const Math::Transform& Transform)
{
	Hlsl::Mesh Mesh = {};
	XMStoreFloat4x4(&Mesh.Transform, XMMatrixTranspose(Transform.Matrix()));
	return Mesh;
}

inline Hlsl::Camera GetHLSLCameraDesc(const CameraComponent& Camera)
{
	using namespace DirectX;

	Hlsl::Camera HlslCamera = {};

	HlslCamera.FoVY		   = Camera.FoVY;
	HlslCamera.AspectRatio = Camera.AspectRatio;
	HlslCamera.NearZ	   = Camera.NearZ;
	HlslCamera.FarZ		   = Camera.FarZ;

	HlslCamera.FocalLength		= Camera.FocalLength;
	HlslCamera.RelativeAperture = Camera.RelativeAperture;

	HlslCamera.Position = {
		Camera.Transform.Position.x,
		Camera.Transform.Position.y,
		Camera.Transform.Position.z,
		1.0f
	};

	XMStoreFloat4x4(&HlslCamera.View, XMMatrixTranspose(XMLoadFloat4x4(&Camera.View)));
	XMStoreFloat4x4(&HlslCamera.Projection, XMMatrixTranspose(XMLoadFloat4x4(&Camera.Projection)));
	XMStoreFloat4x4(&HlslCamera.ViewProjection, XMMatrixTranspose(XMLoadFloat4x4(&Camera.ViewProjection)));

	XMStoreFloat4x4(&HlslCamera.InvView, XMMatrixTranspose(XMLoadFloat4x4(&Camera.InverseView)));
	XMStoreFloat4x4(&HlslCamera.InvProjection, XMMatrixTranspose(XMLoadFloat4x4(&Camera.InverseProjection)));
	XMStoreFloat4x4(&HlslCamera.InvViewProjection, XMMatrixTranspose(XMLoadFloat4x4(&Camera.InverseViewProjection)));

	XMStoreFloat4x4(&HlslCamera.PrevViewProjection, XMMatrixTranspose(XMLoadFloat4x4(&Camera.PrevViewProjection)));

	HlslCamera.Frustum = Math::Frustum(Camera.ViewProjection);

	return HlslCamera;
}

struct View
{
	unsigned int Width, Height;
};

struct WorldRenderView
{
	WorldRenderView(RHI::D3D12LinkedDevice* Device)
		: Lights(Device, sizeof(Hlsl::Light) * World::LightLimit, sizeof(Hlsl::Light), D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_FLAG_NONE)
		, Materials(Device, sizeof(Hlsl::Material) * World::MaterialLimit, sizeof(Hlsl::Material), D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_FLAG_NONE)
		, Meshes(Device, sizeof(Hlsl::Mesh) * World::MeshLimit, sizeof(Hlsl::Mesh), D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_FLAG_NONE)
		, pLights(Lights.GetCpuVirtualAddress<Hlsl::Light>())
		, pMaterial(Materials.GetCpuVirtualAddress<Hlsl::Material>())
		, pMeshes(Meshes.GetCpuVirtualAddress<Hlsl::Mesh>())
	{
	}

	void Update(World* World, /*Optional*/ RaytracingAccelerationStructure* RaytracingAccelerationStructure)
	{
		NumMaterials = NumLights = NumMeshes = 0;
		if (RaytracingAccelerationStructure)
		{
			RaytracingAccelerationStructure->Reset();
		}

		World->Registry.view<CoreComponent, LightComponent>().each(
			[&](CoreComponent& Core, LightComponent& Light)
			{
				pLights[NumLights++] = GetHLSLLightDesc(Core.Transform, Light);
			});
		World->Registry.view<CoreComponent, StaticMeshComponent>().each(
			[&](CoreComponent& Core, StaticMeshComponent& StaticMesh)
			{
				if (StaticMesh.Mesh)
				{
					RHI::D3D12Buffer& VertexBuffer = StaticMesh.Mesh->VertexResource;
					RHI::D3D12Buffer& IndexBuffer  = StaticMesh.Mesh->IndexResource;

					D3D12_DRAW_INDEXED_ARGUMENTS DrawIndexedArguments = {};
					DrawIndexedArguments.IndexCountPerInstance		  = StaticMesh.Mesh->NumIndices;
					DrawIndexedArguments.InstanceCount				  = 1;
					DrawIndexedArguments.StartIndexLocation			  = 0;
					DrawIndexedArguments.BaseVertexLocation			  = 0;
					DrawIndexedArguments.StartInstanceLocation		  = 0;

					Hlsl::Mesh Mesh	  = GetHLSLMeshDesc(Core.Transform);
					Mesh.VertexBuffer = VertexBuffer.GetVertexBufferView();
					Mesh.IndexBuffer  = IndexBuffer.GetIndexBufferView();
					if (StaticMesh.Mesh->Options.GenerateMeshlets)
					{
						Mesh.Meshlets			 = StaticMesh.Mesh->MeshletResource.GetGpuVirtualAddress();
						Mesh.UniqueVertexIndices = StaticMesh.Mesh->UniqueVertexIndexResource.GetGpuVirtualAddress();
						Mesh.PrimitiveIndices	 = StaticMesh.Mesh->PrimitiveIndexResource.GetGpuVirtualAddress();
					}
					Mesh.BoundingBox		  = StaticMesh.Mesh->BoundingBox;
					Mesh.DrawIndexedArguments = DrawIndexedArguments;
					Mesh.MaterialIndex		  = NumMaterials;
					Mesh.NumMeshlets		  = StaticMesh.Mesh->NumMeshlets;
					Mesh.VertexView			  = StaticMesh.Mesh->VertexView.GetIndex();
					Mesh.IndexView			  = StaticMesh.Mesh->IndexView.GetIndex();

					pMaterial[NumMaterials] = GetHLSLMaterialDesc(StaticMesh.Material);
					pMeshes[NumMeshes]		= Mesh;
					if (RaytracingAccelerationStructure)
					{
						RaytracingAccelerationStructure->AddInstance(Core.Transform, &StaticMesh);
					}

					++NumMaterials;
					++NumMeshes;
				}
			});
	}

	RHI::D3D12Buffer Lights;
	RHI::D3D12Buffer Materials;
	RHI::D3D12Buffer Meshes;

	u32 NumLights = 0, NumMaterials = 0, NumMeshes = 0;

	Hlsl::Light*	pLights	  = nullptr;
	Hlsl::Material* pMaterial = nullptr;
	Hlsl::Mesh*		pMeshes	  = nullptr;

	// Set explicitly
	View			 View	= {};
	CameraComponent* Camera = nullptr;
};

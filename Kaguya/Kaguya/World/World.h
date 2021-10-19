#pragma once
#include <entt.hpp>
#include "Components.h"

class Entity;

enum EWorldState
{
	EWorldState_Render,
	EWorldState_Update
};
DEFINE_ENUM_FLAG_OPERATORS(EWorldState);

class World
{
public:
	static constexpr size_t MaterialLimit = 8192;
	static constexpr size_t LightLimit	  = 32;
	static constexpr size_t MeshLimit	  = 8192;

	World() { AddDefaultEntities(); }

	[[nodiscard]] auto CreateEntity(std::string_view Name = {}) -> Entity;
	[[nodiscard]] auto GetMainCamera() -> Entity;
	[[nodiscard]] auto GetEntityByTag(std::string_view Name) -> Entity;

	void Clear(bool bAddDefaultEntities = true);

	void DestroyEntity(Entity Entity);

	template<typename T>
	void OnComponentAdded(Entity Entity, T& Component);

	template<typename T>
	void OnComponentRemoved(Entity Entity, T& Component);

	void Update(float dt);

private:
	void ResolveComponentDependencies();
	void UpdateScripts(float dt);
	void UpdatePhysics(float dt);

	void AddDefaultEntities();

public:
	EWorldState	   WorldState = EWorldState::EWorldState_Render;
	entt::registry Registry;
	Camera*		   ActiveCamera = nullptr;
};

namespace Hlsl
{
struct Material
{
	unsigned int	  BSDFType;
	DirectX::XMFLOAT3 baseColor;
	float			  metallic;
	float			  subsurface;
	float			  specular;
	float			  roughness;
	float			  specularTint;
	float			  anisotropic;
	float			  sheen;
	float			  sheenTint;
	float			  clearcoat;
	float			  clearcoatGloss;

	// Used by Glass BxDF
	DirectX::XMFLOAT3 T;
	float			  etaA, etaB;

	int Albedo;
};

struct Light
{
	unsigned int	  Type;
	DirectX::XMFLOAT3 Position;
	DirectX::XMFLOAT4 Orientation;
	float			  Width;
	float			  Height;
	DirectX::XMFLOAT3 Points[4]; // World-space points that are pre-computed on the Cpu so we don't have to compute them
								 // in shader for every ray

	DirectX::XMFLOAT3 I;
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
	BoundingBox BoundingBox;
	// 20
	D3D12_DRAW_INDEXED_ARGUMENTS DrawIndexedArguments;

	// 20
	unsigned int MaterialIndex;
	unsigned int NumMeshlets;
	unsigned int DEADBEEF0 = 0xDEADBEEF;
	unsigned int DEADBEEF1 = 0xDEADBEEF;
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

	Frustum Frustum;
};
} // namespace Hlsl

inline Hlsl::Material GetHLSLMaterialDesc(const Material& Material)
{
	return { .BSDFType		 = (unsigned int)Material.BSDFType,
			 .baseColor		 = Material.baseColor,
			 .metallic		 = Material.metallic,
			 .subsurface	 = Material.subsurface,
			 .specular		 = Material.specular,
			 .roughness		 = Material.roughness,
			 .specularTint	 = Material.specularTint,
			 .anisotropic	 = Material.anisotropic,
			 .sheen			 = Material.sheen,
			 .sheenTint		 = Material.sheenTint,
			 .clearcoat		 = Material.clearcoat,
			 .clearcoatGloss = Material.clearcoatGloss,

			 .T	   = Material.T,
			 .etaA = Material.etaA,
			 .etaB = Material.etaB,

			 .Albedo = Material.TextureIndices[0] };
}

inline Hlsl::Light GetHLSLLightDesc(const Transform& Transform, const Light& Light)
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

	return { .Type		  = (unsigned int)Light.Type,
			 .Position	  = Transform.Position,
			 .Orientation = Orientation,
			 .Width		  = Light.Width,
			 .Height	  = Light.Height,
			 .Points	  = { Points[0], Points[1], Points[2], Points[3] },

			 .I = Light.I };
}

inline Hlsl::Mesh GetHLSLMeshDesc(const Transform& Transform)
{
	Hlsl::Mesh Mesh = {};
	XMStoreFloat4x4(&Mesh.Transform, XMMatrixTranspose(Transform.Matrix()));
	return Mesh;
}

inline Hlsl::Camera GetHLSLCameraDesc(const Camera& Camera)
{
	using namespace DirectX;

	Hlsl::Camera HlslCamera = {};

	HlslCamera.FoVY		   = Camera.FoVY;
	HlslCamera.AspectRatio = Camera.AspectRatio;
	HlslCamera.NearZ	   = Camera.NearZ;
	HlslCamera.FarZ		   = Camera.FarZ;

	HlslCamera.FocalLength		= Camera.FocalLength;
	HlslCamera.RelativeAperture = Camera.RelativeAperture;

	HlslCamera.Position = { Camera.pTransform->Position.x,
							Camera.pTransform->Position.y,
							Camera.pTransform->Position.z,
							1.0f };

	XMStoreFloat4x4(&HlslCamera.View, XMMatrixTranspose(XMLoadFloat4x4(&Camera.View)));
	XMStoreFloat4x4(&HlslCamera.Projection, XMMatrixTranspose(XMLoadFloat4x4(&Camera.Projection)));
	XMStoreFloat4x4(&HlslCamera.ViewProjection, XMMatrixTranspose(XMLoadFloat4x4(&Camera.ViewProjection)));

	XMStoreFloat4x4(&HlslCamera.InvView, XMMatrixTranspose(XMLoadFloat4x4(&Camera.InverseView)));
	XMStoreFloat4x4(&HlslCamera.InvProjection, XMMatrixTranspose(XMLoadFloat4x4(&Camera.InverseProjection)));
	XMStoreFloat4x4(&HlslCamera.InvViewProjection, XMMatrixTranspose(XMLoadFloat4x4(&Camera.InverseViewProjection)));

	XMStoreFloat4x4(&HlslCamera.PrevViewProjection, XMMatrixTranspose(XMLoadFloat4x4(&Camera.PrevViewProjection)));

	HlslCamera.Frustum = Frustum(Camera.ViewProjection);

	return HlslCamera;
}

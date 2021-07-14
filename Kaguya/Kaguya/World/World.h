#pragma once
#include <entt.hpp>
#include <string_view>
#include "Components.h"

struct Entity;

enum EWorldState
{
	EWorldState_Render,
	EWorldState_Update
};
DEFINE_ENUM_FLAG_OPERATORS(EWorldState);

class World
{
public:
	static constexpr UINT64 MaterialLimit = 1024;
	static constexpr UINT64 LightLimit	  = 100;
	static constexpr UINT64 InstanceLimit = 1024;

	World() { Clear(); }

	Entity GetMainCamera();

	void Clear();

	Entity CreateEntity(std::string_view Name);

	void DestroyEntity(Entity Entity);

	Entity GetEntityByTag(std::string_view Name);

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

namespace HLSL
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
	unsigned int			  VertexOffset;
	unsigned int			  IndexOffset;
	unsigned int			  MaterialIndex;
	unsigned int			  InstanceID						  : 24;
	unsigned int			  InstanceMask						  : 8;
	unsigned int			  InstanceContributionToHitGroupIndex : 24;
	unsigned int			  Flags								  : 8;
	D3D12_GPU_VIRTUAL_ADDRESS AccelerationStructure;
	DirectX::XMFLOAT4X4		  World;
	DirectX::XMFLOAT4X4		  PreviousWorld;
	DirectX::XMFLOAT3X4		  Transform;
};

struct Camera
{
	float NearZ;
	float FarZ;
	float FocalLength;
	float RelativeAperture;

	DirectX::XMFLOAT4 Position;
	DirectX::XMFLOAT4 U;
	DirectX::XMFLOAT4 V;
	DirectX::XMFLOAT4 W;

	DirectX::XMFLOAT4X4 View;
	DirectX::XMFLOAT4X4 Projection;
	DirectX::XMFLOAT4X4 ViewProjection;
	DirectX::XMFLOAT4X4 InvView;
	DirectX::XMFLOAT4X4 InvProjection;
	DirectX::XMFLOAT4X4 InvViewProjection;
};
} // namespace HLSL

inline HLSL::Material GetHLSLMaterialDesc(const Material& Material)
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

inline HLSL::Light GetHLSLLightDesc(const Transform& Transform, const Light& Light)
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

inline HLSL::Camera GetHLSLCameraDesc(const Camera& Camera)
{
	using namespace DirectX;

	XMFLOAT4   Position = { Camera.pTransform->Position.x,
							Camera.pTransform->Position.y,
							Camera.pTransform->Position.z,
							1.0f };
	XMFLOAT4   U, V, W;
	XMFLOAT4X4 View, Projection, ViewProjection, InvView, InvProjection, InvViewProjection;

	XMStoreFloat4(&U, Camera.GetUVector());
	XMStoreFloat4(&V, Camera.GetVVector());
	XMStoreFloat4(&W, Camera.GetWVector());

	XMStoreFloat4x4(&View, XMMatrixTranspose(Camera.ViewMatrix));
	XMStoreFloat4x4(&Projection, XMMatrixTranspose(Camera.ProjectionMatrix));
	XMStoreFloat4x4(&ViewProjection, XMMatrixTranspose(Camera.ViewProjectionMatrix));
	XMStoreFloat4x4(&InvView, XMMatrixTranspose(Camera.InverseViewMatrix));
	XMStoreFloat4x4(&InvProjection, XMMatrixTranspose(Camera.InverseProjectionMatrix));
	XMStoreFloat4x4(&InvViewProjection, XMMatrixTranspose(Camera.InverseViewProjectionMatrix));

	return { .NearZ			   = Camera.NearZ,
			 .FarZ			   = Camera.FarZ,
			 .FocalLength	   = Camera.FocalLength,
			 .RelativeAperture = Camera.RelativeAperture,

			 .Position = Position,
			 .U		   = U,
			 .V		   = V,
			 .W		   = W,

			 .View				= View,
			 .Projection		= Projection,
			 .ViewProjection	= ViewProjection,
			 .InvView			= InvView,
			 .InvProjection		= InvProjection,
			 .InvViewProjection = InvViewProjection };
}

#pragma once
#include <entt.hpp>
#include "Components.h"
#include "Actor.h"

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

	World();

	[[nodiscard]] auto CreateActor(std::string_view Name = {}) -> Actor;
	[[nodiscard]] auto GetMainCamera() -> Actor;

	void Clear(bool AddDefaultEntities = true);

	void DestroyActor(size_t Index);
	void CloneActor(size_t Index);

	template<typename T>
	void OnComponentAdded(Actor Actor, T& Component);

	template<typename T>
	void OnComponentRemoved(Actor Actor, T& Component);

	void Update(float DeltaTime);

	void BeginPlay();

private:
	void ResolveComponentDependencies();
	void UpdateScripts(float DeltaTime);

public:
	EWorldState		   WorldState = EWorldState::EWorldState_Render;
	entt::registry	   Registry;
	CameraComponent*   ActiveCamera = nullptr;
	std::vector<Actor> Actors;
};

template<typename T, typename... TArgs>
auto Actor::AddComponent(TArgs&&... Args) -> T&
{
	assert(!HasComponent<T>());

	T& Component = World->Registry.emplace<T>(Handle, std::forward<TArgs>(Args)...);
	World->OnComponentAdded<T>(*this, Component);
	return Component;
}

template<typename T>
[[nodiscard]] auto Actor::GetComponent() -> T&
{
	assert(HasComponent<T>());

	return World->Registry.get<T>(Handle);
}

template<typename T, typename... TArgs>
[[nodiscard]] auto Actor::GetOrAddComponent(TArgs&&... Args) -> T&
{
	if (HasComponent<T>())
	{
		return GetComponent<T>();
	}
	return AddComponent<T>(std::forward<TArgs>(Args)...);
}

template<typename T>
[[nodiscard]] auto Actor::HasComponent() -> bool
{
	return World->Registry.any_of<T>(Handle);
}

template<typename T>
void Actor::RemoveComponent()
{
	assert(HasComponent<T>());

	T& Component = GetComponent<T>();
	World->OnComponentRemoved<T>(*this, Component);

	World->Registry.remove<T>(Handle);
}

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

inline Hlsl::Light GetHLSLLightDesc(const Transform& Transform, const LightComponent& Light)
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

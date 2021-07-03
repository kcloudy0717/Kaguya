#pragma once
#include <entt.hpp>

#include "Camera.h"
#include "CameraController.h"
#include "Components.h"

#include "../SharedTypes.h"

struct Entity;

enum ESceneState
{
	SceneState_Render,
	SceneState_Update
};
DEFINE_ENUM_FLAG_OPERATORS(ESceneState);

struct Scene
{
	static constexpr UINT64 MaterialLimit = 1024;
	static constexpr UINT64 LightLimit	  = 100;
	static constexpr UINT64 InstanceLimit = 1024;

	Scene() noexcept;

	void Clear();

	void Update(float dt);

	Entity CreateEntity(const std::string& Name);
	void   DestroyEntity(Entity Entity);

	template<typename T>
	void OnComponentAdded(Entity Entity, T& Component);

	ESceneState	   SceneState = SceneState_Render;
	entt::registry Registry;

	Camera							  Camera, PreviousCamera;
	std::unique_ptr<CameraController> CameraController;
};

inline HLSL::Material GetHLSLMaterialDesc(const Material& Material)
{
	return { .BSDFType		 = Material.BSDFType,
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

			 .TextureIndices = { Material.TextureIndices[0],
								 Material.TextureIndices[1],
								 Material.TextureIndices[2],
								 Material.TextureIndices[3] },
			 .TextureChannel = { Material.TextureChannel[0],
								 Material.TextureChannel[1],
								 Material.TextureChannel[2],
								 Material.TextureChannel[3] } };
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

	return { .Type		  = (uint)Light.Type,
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

	XMFLOAT4 Position = { Camera.Transform.Position.x, Camera.Transform.Position.y, Camera.Transform.Position.z, 1.0f };
	XMFLOAT4 U, V, W;
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

	return { .NearZ		= Camera.NearZ,
			 .FarZ		= Camera.FarZ,
			 ._padding0 = 0.0f,
			 ._padding1 = 0.0f,

			 .FocalLength		= Camera.FocalLength,
			 .RelativeAperture	= Camera.RelativeAperture,
			 .ShutterTime		= Camera.ShutterTime,
			 .SensorSensitivity = Camera.SensorSensitivity,

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

#pragma once
#include <type_traits>
#include <string>
#include <entt.hpp>
#include <DirectXMath.h>

#include "../SharedDefines.h"
#include "../Asset/Image.h"
#include "../Asset/Mesh.h"
#include "../Asset/AssetCache.h"

struct Component
{
	entt::entity  Handle   = entt::null;
	struct Scene* pScene   = nullptr;
	bool		  IsEdited = false;
};

template<class T>
concept is_component = std::is_base_of_v<Component, T>;

struct Tag : Component
{
	Tag() = default;
	Tag(std::string_view Name)
		: Name(Name)
	{
	}

	operator std::string&() { return Name; }
	operator const std::string&() const { return Name; }

	std::string Name;
};

struct Transform : Component
{
	Transform();

	void SetTransform(DirectX::FXMMATRIX M);

	void Translate(float DeltaX, float DeltaY, float DeltaZ);

	void SetScale(float ScaleX, float ScaleY, float ScaleZ);

	// Radians
	void SetOrientation(float AngleX, float AngleY, float AngleZ);

	void Rotate(float AngleX, float AngleY, float AngleZ);

	DirectX::XMMATRIX Matrix() const;

	DirectX::XMVECTOR Right() const;

	DirectX::XMVECTOR Up() const;

	DirectX::XMVECTOR Forward() const;

	bool operator==(const Transform& Transform) const;
	bool operator!=(const Transform& Transform) const;

	DirectX::XMFLOAT3 Position;
	DirectX::XMFLOAT3 Scale;
	DirectX::XMFLOAT4 Orientation;

	bool UseSnap			   = false;
	UINT CurrentGizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
};

struct Light : Component
{
	Light();

	enum EType
	{
		Point,
		Quad
	} Type;

	DirectX::XMFLOAT3 I;
	// Used by QuadLight
	float Width, Height;
};

struct MeshFilter : Component
{
	// Key into the MeshCache
	UINT64					 Key  = 0;
	AssetHandle<Asset::Mesh> Mesh = {};
};

enum TextureTypes
{
	AlbedoIdx,
	NormalIdx,
	RoughnessIdx,
	MetallicIdx,
	NumTextureTypes
};

#define TEXTURE_CHANNEL_RED	  0
#define TEXTURE_CHANNEL_GREEN 1
#define TEXTURE_CHANNEL_BLUE  2
#define TEXTURE_CHANNEL_ALPHA 3

enum BSDFTypes
{
	Lambertian,
	Mirror,
	Glass,
	Disney,
	NumBSDFTypes
};

struct Material
{
	Material()
	{
		for (int i = 0; i < NumTextureTypes; ++i)
		{
			TextureIndices[i] = -1;
			TextureChannel[i] = 0;

			TextureKeys[i] = 0;
		}
	}

	int	   BSDFType		  = BSDFTypes::Disney;
	float3 baseColor	  = { 1, 1, 1 };
	float  metallic		  = 0.0f;
	float  subsurface	  = 0.0f;
	float  specular		  = 0.5f;
	float  roughness	  = 0.5f;
	float  specularTint	  = 0.0f;
	float  anisotropic	  = 0.0f;
	float  sheen		  = 0.0f;
	float  sheenTint	  = 0.5f;
	float  clearcoat	  = 0.0f;
	float  clearcoatGloss = 1.0f;

	// Used by Glass BxDF
	float3 T	= { 1, 1, 1 };
	float  etaA = 1.000277f; // air
	float  etaB = 1.5046f;	 // glass

	int TextureIndices[NumTextureTypes];
	int TextureChannel[NumTextureTypes];

	UINT64					  TextureKeys[NumTextureTypes];
	AssetHandle<Asset::Image> Textures[NumTextureTypes];
};

struct MeshRenderer : Component
{
	MeshFilter* pMeshFilter = nullptr;
	Material	Material;
};

#pragma once

enum ETextureTypes
{
	AlbedoIdx,
	NumTextureTypes
};

enum class EBSDFTypes
{
	Lambertian,
	Mirror,
	Glass,
	Disney,
	NumBSDFTypes
};

struct MaterialTexture
{
	std::string				  Path;
	UINT64					  Key;
	AssetHandle<Asset::Image> Image;
};

struct Material
{
	Material()
	{
		for (int& TextureIndice : TextureIndices)
		{
			TextureIndice = -1;
		}
	}

	EBSDFTypes		  BSDFType		 = EBSDFTypes::Disney;
	DirectX::XMFLOAT3 baseColor		 = { 1, 1, 1 };
	float			  metallic		 = 0.0f;
	float			  subsurface	 = 0.0f;
	float			  specular		 = 0.5f;
	float			  roughness		 = 0.5f;
	float			  specularTint	 = 0.0f;
	float			  anisotropic	 = 0.0f;
	float			  sheen			 = 0.0f;
	float			  sheenTint		 = 0.5f;
	float			  clearcoat		 = 0.0f;
	float			  clearcoatGloss = 1.0f;

	// Used by Glass BxDF
	DirectX::XMFLOAT3 T	   = { 1, 1, 1 };
	float			  etaA = 1.000277f; // air
	float			  etaB = 1.5046f;	// glass

	MaterialTexture Albedo;

	int TextureIndices[ETextureTypes::NumTextureTypes];
};

struct MeshRenderer : Component
{
	MeshFilter* pMeshFilter = nullptr;
	Material	Material;
};

REGISTER_CLASS_ATTRIBUTES(MaterialTexture, CLASS_ATTRIBUTE(MaterialTexture, Path))

REGISTER_CLASS_ATTRIBUTES(
	Material,
	CLASS_ATTRIBUTE(Material, BSDFType),
	CLASS_ATTRIBUTE(Material, baseColor),
	CLASS_ATTRIBUTE(Material, metallic),
	CLASS_ATTRIBUTE(Material, subsurface),
	CLASS_ATTRIBUTE(Material, specular),
	CLASS_ATTRIBUTE(Material, roughness),
	CLASS_ATTRIBUTE(Material, specularTint),
	CLASS_ATTRIBUTE(Material, anisotropic),
	CLASS_ATTRIBUTE(Material, sheen),
	CLASS_ATTRIBUTE(Material, sheenTint),
	CLASS_ATTRIBUTE(Material, clearcoat),
	CLASS_ATTRIBUTE(Material, clearcoatGloss),
	CLASS_ATTRIBUTE(Material, T),
	CLASS_ATTRIBUTE(Material, etaA),
	CLASS_ATTRIBUTE(Material, etaB),
	CLASS_ATTRIBUTE(Material, Albedo))

REGISTER_CLASS_ATTRIBUTES(
		MeshRenderer,
		CLASS_ATTRIBUTE(MeshRenderer, Material))

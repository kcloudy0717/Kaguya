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
	AssetHandle Handle	 = {};
	uint32_t	HandleId = UINT32_MAX;
	Texture*	Texture	 = nullptr;
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

	EBSDFTypes		  BSDFType		 = EBSDFTypes::Lambertian;
	DirectX::XMFLOAT3 BaseColor		 = { 1, 1, 1 };
	float			  Metallic		 = 0.0f;
	float			  Subsurface	 = 0.0f;
	float			  Specular		 = 0.5f;
	float			  Roughness		 = 0.5f;
	float			  SpecularTint	 = 0.0f;
	float			  Anisotropic	 = 0.0f;
	float			  Sheen			 = 0.0f;
	float			  SheenTint		 = 0.5f;
	float			  Clearcoat		 = 0.0f;
	float			  ClearcoatGloss = 1.0f;

	// Used by Glass BxDF
	DirectX::XMFLOAT3 T	   = { 1, 1, 1 };
	float			  EtaA = 1.000277f; // air
	float			  EtaB = 1.5046f;	// glass

	MaterialTexture Albedo;

	int TextureIndices[ETextureTypes::NumTextureTypes];
};

class StaticMeshComponent
{
public:
	AssetHandle Handle	 = {};
	uint32_t	HandleId = UINT32_MAX;
	Mesh*		Mesh	 = nullptr;

	Material Material;
};

REGISTER_CLASS_ATTRIBUTES(MaterialTexture, "MaterialTexture", CLASS_ATTRIBUTE(MaterialTexture, HandleId))

REGISTER_CLASS_ATTRIBUTES(
	Material,
	"Material",
	CLASS_ATTRIBUTE(Material, BSDFType),
	CLASS_ATTRIBUTE(Material, BaseColor),
	CLASS_ATTRIBUTE(Material, Metallic),
	CLASS_ATTRIBUTE(Material, Subsurface),
	CLASS_ATTRIBUTE(Material, Specular),
	CLASS_ATTRIBUTE(Material, Roughness),
	CLASS_ATTRIBUTE(Material, SpecularTint),
	CLASS_ATTRIBUTE(Material, Anisotropic),
	CLASS_ATTRIBUTE(Material, Sheen),
	CLASS_ATTRIBUTE(Material, SheenTint),
	CLASS_ATTRIBUTE(Material, Clearcoat),
	CLASS_ATTRIBUTE(Material, ClearcoatGloss),
	CLASS_ATTRIBUTE(Material, T),
	CLASS_ATTRIBUTE(Material, EtaA),
	CLASS_ATTRIBUTE(Material, EtaB),
	CLASS_ATTRIBUTE(Material, Albedo))

REGISTER_CLASS_ATTRIBUTES(
	StaticMeshComponent,
	"StaticMesh",
	CLASS_ATTRIBUTE(StaticMeshComponent, HandleId),
	CLASS_ATTRIBUTE(StaticMeshComponent, Material))

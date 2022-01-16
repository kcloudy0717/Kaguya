#pragma once
#include "Asset.h"
#include "Math/Math.h"
#include "RHI/RHI.h"
#include <DirectXTex.h>

struct TextureImportOptions
{
	std::filesystem::path Path;

	bool sRGB		  = false;
	bool GenerateMips = true;
};

class Texture : public Asset
{
public:
	void Release()
	{
		TexImage.Release();
	}

	TextureImportOptions Options;

	Vec2i Resolution;
	bool  IsCubemap = false;

	std::string			  Name;
	DirectX::ScratchImage TexImage;

	D3D12Texture			DxTexture;
	D3D12ShaderResourceView SRV;
};

template<>
struct AssetTypeTraits<AssetType::Texture>
{
	using Type = Texture;
};

#pragma once
#include "Asset.h"
#include <DirectXTex.h>
#include <Core/RHI/D3D12/D3D12Device.h>

struct TextureImportOptions
{
	std::filesystem::path Path;

	bool sRGB		  = false;
	bool GenerateMips = true;
};

class Texture : public Asset
{
public:
	TextureImportOptions Options;

	Vector2i Resolution;

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

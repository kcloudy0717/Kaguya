#pragma once
#include <DirectXTex.h>
#include <Core/RHI/D3D12/D3D12Device.h>

struct TextureMetadata
{
	std::filesystem::path Path;
	bool				  sRGB;
	bool				  GenerateMips = true;
};

class Texture : public Asset
{
public:
	TextureMetadata Metadata;

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

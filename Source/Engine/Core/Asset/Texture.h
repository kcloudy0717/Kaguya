#pragma once
#include "Asset.h"
#include <DirectXTex.h>
#include "Core/Math/Math.h"
#include "Core/RHI/D3D12/D3D12Descriptor.h"
#include "Core/RHI/D3D12/D3D12Device.h"

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

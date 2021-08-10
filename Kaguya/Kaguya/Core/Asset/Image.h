#pragma once
#include <DirectXTex.h>
#include "D3D12/D3D12Device.h"

namespace Asset
{
struct Image;

struct ImageMetadata
{
	std::filesystem::path Path;
	bool				  sRGB;
};

struct Image
{
	ImageMetadata Metadata;

	Vector2i Resolution;

	std::string			  Name;
	DirectX::ScratchImage Image;

	D3D12Texture			Texture;
	D3D12ShaderResourceView SRV;
};
} // namespace Asset

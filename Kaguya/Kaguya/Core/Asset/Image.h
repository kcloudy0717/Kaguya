#pragma once
#include <DirectXTex.h>
#include <Core/D3D12/Adapter.h>

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

	Texture			   Texture;
	ShaderResourceView SRV;
};
} // namespace Asset

#pragma once
#include <string>
#include <DirectXTex.h>
#include "../RenderDevice.h"

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

	Texture Texture;
};
} // namespace Asset

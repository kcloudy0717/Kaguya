#pragma once
#include "Core/Asset/IAsset.h"
#include "Core/Asset/Texture.h"

class SkyLightComponent
{
public:
	Asset::AssetHandle Handle	= {};
	uint32_t		   HandleId = UINT32_MAX;
	Asset::Texture*	   Texture	= nullptr;

	DirectX::XMFLOAT3 I = { 1.0f, 1.0f, 1.0f }; // Intensity of the light

	int SRVIndex = -1;

	bool Main = true;
};

REGISTER_CLASS_ATTRIBUTES(
	SkyLightComponent,
	"SkyLight",
	CLASS_ATTRIBUTE(SkyLightComponent, HandleId),
	CLASS_ATTRIBUTE(SkyLightComponent, I))

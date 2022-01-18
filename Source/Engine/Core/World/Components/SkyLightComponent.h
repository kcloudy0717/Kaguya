#pragma once
#include "Core/Asset/Asset.h"
#include "Core/Asset/Texture.h"

class SkyLightComponent
{
public:
	AssetHandle Handle	 = {};
	uint32_t	HandleId = UINT32_MAX;
	Texture*	Texture	 = nullptr;

	DirectX::XMFLOAT3 I = { 1.0f, 1.0f, 1.0f }; // Intensity of the light

	int SRVIndex = -1;

	bool Main = true;
};

REGISTER_CLASS_ATTRIBUTES(
	SkyLightComponent,
	"SkyLight",
	CLASS_ATTRIBUTE(SkyLightComponent, HandleId),
	CLASS_ATTRIBUTE(SkyLightComponent, I))

#pragma once
#include "Component.h"
#include "../../SharedDefines.h"
#include "../../Asset/Image.h"
#include "../../Asset/AssetCache.h"

enum LightType
{
	PointLight,
	QuadLight
};

struct Light : Component
{
	__forceinline Light()
	{
		Type = PointLight;
		I = { 1.0f, 1.0f, 1.0f };
		Width = Height = 1.0f;
	}

	LightType Type;

	float3 I;

	// Used by QuadLight
	float Width, Height;
};
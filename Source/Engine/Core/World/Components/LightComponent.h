#pragma once

enum class ELightTypes
{
	Point,
	Quad
};

struct LightComponent
{
	ELightTypes		  Type	= ELightTypes::Point;
	DirectX::XMFLOAT3 I		= { 1.0f, 1.0f, 1.0f }; // Intensity of the light
	float			  Width = 1.0f, Height = 1.0f;	// Used by QuadLight
};

REGISTER_CLASS_ATTRIBUTES(
	LightComponent,
	"Light",
	CLASS_ATTRIBUTE(LightComponent, Type),
	CLASS_ATTRIBUTE(LightComponent, I),
	CLASS_ATTRIBUTE(LightComponent, Width),
	CLASS_ATTRIBUTE(LightComponent, Height))

#pragma once

enum class ELightTypes
{
	Point,
	Quad
};

class LightComponent
{
public:
	ELightTypes		  Type = ELightTypes::Point;
	DirectX::XMFLOAT3 I;			 // Intensity of the light
	float			  Width, Height; // Used by QuadLight
};

REGISTER_CLASS_ATTRIBUTES(
	LightComponent,
	"Light",
	CLASS_ATTRIBUTE(LightComponent, Type),
	CLASS_ATTRIBUTE(LightComponent, I),
	CLASS_ATTRIBUTE(LightComponent, Width),
	CLASS_ATTRIBUTE(LightComponent, Height))

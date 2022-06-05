#pragma once

enum class ELightTypes
{
	Point,
	Quad,
	Directional,
	Spot
};

struct LightComponent
{
	ELightTypes		  Type = ELightTypes::Point;
	DirectX::XMFLOAT3 I	   = { 1.0f, 1.0f, 1.0f }; // Intensity of the light

	float Width	 = 1.0f;
	float Height = 1.0f; // Used by QuadLight

	float Radius	 = 1.0f;
	float InnerAngle = cosf(DirectX::XMConvertToRadians(2.0f));
	float OuterAngle = cosf(DirectX::XMConvertToRadians(25.0f));
};

REGISTER_CLASS_ATTRIBUTES(
	LightComponent,
	"Light",
	CLASS_ATTRIBUTE(LightComponent, Type),
	CLASS_ATTRIBUTE(LightComponent, I),
	CLASS_ATTRIBUTE(LightComponent, Width),
	CLASS_ATTRIBUTE(LightComponent, Height))

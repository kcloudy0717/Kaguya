#pragma once

enum class ELightTypes
{
	Point,
	Quad
};

struct Light : Component
{
	ELightTypes		  Type = ELightTypes::Point;
	DirectX::XMFLOAT3 I;			 // Intensity of the light
	float			  Width, Height; // Used by QuadLight
};

REGISTER_CLASS_ATTRIBUTES(
	Light,
	CLASS_ATTRIBUTE(Light, Type),
	CLASS_ATTRIBUTE(Light, I),
	CLASS_ATTRIBUTE(Light, Width),
	CLASS_ATTRIBUTE(Light, Height))

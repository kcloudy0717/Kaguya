#pragma once
#include "Math.h"

// ax + by + cz = d where d = dot(n, P)
struct Plane
{
	Plane() noexcept = default;
	explicit Plane(Vec3f a, Vec3f b, Vec3f c) noexcept;

	void Normalize() noexcept;

	Vec3f Normal;		 // (a,b,c)
	float Offset = 0.0f; // d
};

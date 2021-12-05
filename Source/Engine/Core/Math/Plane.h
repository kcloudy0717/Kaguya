#pragma once
#include "Vector3.h"

// ax + by + cz = d where d = dot(n, P)
struct Plane
{
	Plane() noexcept = default;
	explicit Plane(Vector3f a, Vector3f b, Vector3f c) noexcept;

	void Normalize() noexcept;

	Vector3f Normal;		// (a,b,c)
	float	 Offset = 0.0f; // d
};

#pragma once
#include "MathCore.h"
#include "Vec3.h"

// ax + by + cz = d where d = dot(n, P)
struct Plane
{
	Plane() noexcept = default;
	explicit Plane(const Vec3f& a, const Vec3f& b, const Vec3f& c) noexcept;

	Vec3f Normal;		 // (a,b,c)
	float Offset = 0.0f; // d
};

inline Plane::Plane(const Vec3f& a, const Vec3f& b, const Vec3f& c) noexcept
{
	Normal = normalize(cross(b - a, c - a));
	Offset = dot(Normal, a);
}

inline Plane normalize(Plane plane)
{
	float reciprocalMag = 1.0f / length(plane.Normal);
	plane.Normal *= reciprocalMag;
	plane.Offset *= reciprocalMag;
	return plane;
}

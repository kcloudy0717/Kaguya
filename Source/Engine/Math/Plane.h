#pragma once
#include "MathCore.h"
#include "Vec3.h"

// ax + by + cz = d where d = dot(n, P)
struct Plane
{
	Plane() noexcept = default;
	explicit Plane(const Vec3f& V0, const Vec3f& V1, const Vec3f& V2) noexcept;

	Vec3f Normal;		 // (a,b,c)
	float Offset = 0.0f; // d
};

inline Plane::Plane(const Vec3f& V0, const Vec3f& V1, const Vec3f& V2) noexcept
{
	Normal = Normalize(Cross(V1 - V0, V2 - V0));
	Offset = Dot(Normal, V0);
}

inline Plane Normalize(Plane Plane)
{
	float reciprocalMag = 1.0f / Length(Plane.Normal);
	Plane.Normal *= reciprocalMag;
	Plane.Offset *= reciprocalMag;
	return Plane;
}

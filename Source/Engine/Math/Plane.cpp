#include "Plane.h"

Plane::Plane(Vec3f a, Vec3f b, Vec3f c) noexcept
{
	Normal = normalize(cross(b - a, c - a));
	Offset = dot(Normal, a);
}

void Plane::Normalize() noexcept
{
	float reciprocalMag = 1.0f / length(Normal);
	Normal *= reciprocalMag;
	Offset *= reciprocalMag;
}

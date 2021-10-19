#include "Math.h"

Plane::Plane(Vector3f a, Vector3f b, Vector3f c) noexcept
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

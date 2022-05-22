#pragma once
#include "Vec3.h"

namespace Math
{
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
		Normal = normalize(cross(V1 - V0, V2 - V0));
		Offset = dot(Normal, V0);
	}

	inline Plane Normalize(Plane Plane)
	{
		float ReciprocalMag = 1.0f / length(Plane.Normal);
		Plane.Normal *= ReciprocalMag;
		Plane.Offset *= ReciprocalMag;
		return Plane;
	}
} // namespace Math

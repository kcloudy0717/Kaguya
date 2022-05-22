#pragma once
#include "Vec3.h"

namespace Math
{
	struct Ray
	{
		Vec3f operator()(float T) const noexcept;

		Vec3f Origin	= { 0.0f, 0.0f, 0.0f };
		Vec3f Direction = { 0.0f, 0.0f, 1.0f };
	};

	inline Vec3f Ray::operator()(float T) const noexcept
	{
		return Origin + Direction * T;
	}
} // namespace Math

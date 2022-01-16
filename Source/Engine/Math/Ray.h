#pragma once
#include "MathCore.h"
#include "Vec3.h"

struct Ray
{
	Vec3f operator()(float T) const noexcept;

	Vec3f Origin;
	float TMin = 0.0f;
	Vec3f Direction;
	float TMax = 8.0f;
};

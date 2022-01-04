#pragma once
#include "Math.h"

struct Ray
{
	Vec3f Origin;
	float TMin = 0.0f;
	Vec3f Direction;
	float TMax = 8.0f;
};

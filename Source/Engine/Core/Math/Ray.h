#pragma once
#include "Vector3.h"

struct Ray
{
	Vector3f Origin;
	float	 TMin = 0.0f;
	Vector3f Direction;
	float	 TMax = 8.0f;
};

#pragma once

enum class PlaneIntersection
{
	PositiveHalfspace,
	NegativeHalfspace,
	Intersecting,
};

enum class ContainmentType
{
	Disjoint,
	Intersects,
	Contains
};

#define FRUSTUM_PLANE_LEFT	 0
#define FRUSTUM_PLANE_RIGHT	 1
#define FRUSTUM_PLANE_BOTTOM 2
#define FRUSTUM_PLANE_TOP	 3
#define FRUSTUM_PLANE_NEAR	 4
#define FRUSTUM_PLANE_FAR	 5

#pragma once
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <cmath>
#include <limits>
#include <compare>
#include <algorithm>

// Returns radians
[[nodiscard]] constexpr float operator"" _Deg(long double Degrees)
{
	return DirectX::XMConvertToRadians(static_cast<float>(Degrees));
}

// Returns degrees
[[nodiscard]] constexpr float operator"" _Rad(long double Radians)
{
	return DirectX::XMConvertToDegrees(static_cast<float>(Radians));
}

[[nodiscard]] constexpr float lerp(float x, float y, float s)
{
	return x + s * (y - x);
}

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

#pragma once
#include "MathCore.h"
#include "Vec3.h"
#include "Plane.h"

struct BoundingBox
{
	void GetCorners(
		Vec3f* FarBottomLeft,
		Vec3f* FarBottomRight,
		Vec3f* FarTopRight,
		Vec3f* FarTopLeft,
		Vec3f* NearBottomLeft,
		Vec3f* NearBottomRight,
		Vec3f* NearTopRight,
		Vec3f* NearTopLeft) const noexcept;

	[[nodiscard]] bool				Intersects(const BoundingBox& Other) const noexcept;
	[[nodiscard]] PlaneIntersection Intersects(const Plane& Plane) const noexcept;

	void Transform(DirectX::XMFLOAT4X4 Matrix, BoundingBox& BoundingBox) const noexcept;

	Vec3f Center  = { 0.0f, 0.0f, 0.0f };
	Vec3f Extents = { 1.0f, 1.0f, 1.0f };
};

#pragma once
#include <array>
#include "Math.h"
#include "Plane.h"

struct BoundingBox
{
	[[nodiscard]] std::array<Vec3f, 8> GetCorners() const noexcept;

	[[nodiscard]] bool				Intersects(const BoundingBox& Other) const noexcept;
	[[nodiscard]] PlaneIntersection Intersects(const Plane& Plane) const noexcept;

	void Transform(DirectX::XMFLOAT4X4 Matrix, BoundingBox& BoundingBox) const noexcept;

	Vec3f Center  = { 0.0f, 0.0f, 0.0f };
	Vec3f Extents = { 1.0f, 1.0f, 1.0f };
};

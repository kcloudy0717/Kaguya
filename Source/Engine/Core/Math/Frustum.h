#pragma once
#include "Math.h"
#include "Plane.h"
#include "BoundingBox.h"

// All frustum planes are facing inwards of the frustum, so if any object
// is in the negative halfspace, then is not visible
struct Frustum
{
	Frustum() noexcept = default;
	explicit Frustum(const DirectX::XMFLOAT4X4& Matrix) noexcept;

	[[nodiscard]] ContainmentType Contains(const BoundingBox& Box) const noexcept;

	Plane Left;	  // -x
	Plane Right;  // +x
	Plane Bottom; // -y
	Plane Top;	  // +y
	Plane Near;	  // -z
	Plane Far;	  // +z
};

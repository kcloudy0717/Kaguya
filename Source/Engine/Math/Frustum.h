#pragma once
#include "Types.h"
#include "Plane.h"
#include "BoundingBox.h"

namespace Math
{
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

	inline Frustum::Frustum(const DirectX::XMFLOAT4X4& Matrix) noexcept
	{
		// 1. If Matrix is equal to the projection matrix P, the algorithm gives the
		// clipping planes in view space.

		// 2. If Matrix is equal to the view projection matrix V*P, then the algorithm gives
		// clipping plane in world space.

		// I negate Offset of every plane because I use
		// ax + by + cz = d where d = dot(n, P) as the representation of a plane
		// where as the paper uses ax + by + cz + D = 0 where D = -dot(n, P)

		// Left clipping plane
		Left.Normal.x = Matrix._14 + Matrix._11;
		Left.Normal.y = Matrix._24 + Matrix._21;
		Left.Normal.z = Matrix._34 + Matrix._31;
		Left.Offset	  = -(Matrix._44 + Matrix._41);
		// Right clipping plane
		Right.Normal.x = Matrix._14 - Matrix._11;
		Right.Normal.y = Matrix._24 - Matrix._21;
		Right.Normal.z = Matrix._34 - Matrix._31;
		Right.Offset   = -(Matrix._44 - Matrix._41);
		// Bottom clipping plane
		Bottom.Normal.x = Matrix._14 + Matrix._12;
		Bottom.Normal.y = Matrix._24 + Matrix._22;
		Bottom.Normal.z = Matrix._34 + Matrix._32;
		Bottom.Offset	= -(Matrix._44 + Matrix._42);
		// Top clipping plane
		Top.Normal.x = Matrix._14 - Matrix._12;
		Top.Normal.y = Matrix._24 - Matrix._22;
		Top.Normal.z = Matrix._34 - Matrix._32;
		Top.Offset	 = -(Matrix._44 - Matrix._42);
		// Near clipping plane
		Near.Normal.x = Matrix._13;
		Near.Normal.y = Matrix._23;
		Near.Normal.z = Matrix._33;
		Near.Offset	  = -(Matrix._43);
		// Far clipping plane
		Far.Normal.x = Matrix._14 - Matrix._13;
		Far.Normal.y = Matrix._24 - Matrix._23;
		Far.Normal.z = Matrix._34 - Matrix._33;
		Far.Offset	 = -(Matrix._44 - Matrix._43);

		Left   = Normalize(Left);
		Right  = Normalize(Right);
		Bottom = Normalize(Bottom);
		Top	   = Normalize(Top);
		Near   = Normalize(Near);
		Far	   = Normalize(Far);
	}

	inline ContainmentType Frustum::Contains(const BoundingBox& Box) const noexcept
	{
		PlaneIntersection P0 = Box.Intersects(Left);
		PlaneIntersection P1 = Box.Intersects(Right);
		PlaneIntersection P2 = Box.Intersects(Bottom);
		PlaneIntersection P3 = Box.Intersects(Top);
		PlaneIntersection P4 = Box.Intersects(Near);
		PlaneIntersection P5 = Box.Intersects(Far);

		bool AnyOutside = P0 == PlaneIntersection::NegativeHalfspace;
		AnyOutside |= P1 == PlaneIntersection::NegativeHalfspace;
		AnyOutside |= P2 == PlaneIntersection::NegativeHalfspace;
		AnyOutside |= P3 == PlaneIntersection::NegativeHalfspace;
		AnyOutside |= P4 == PlaneIntersection::NegativeHalfspace;
		AnyOutside |= P5 == PlaneIntersection::NegativeHalfspace;
		bool AllInside = P0 == PlaneIntersection::PositiveHalfspace;
		AllInside &= P1 == PlaneIntersection::PositiveHalfspace;
		AllInside &= P2 == PlaneIntersection::PositiveHalfspace;
		AllInside &= P3 == PlaneIntersection::PositiveHalfspace;
		AllInside &= P4 == PlaneIntersection::PositiveHalfspace;
		AllInside &= P5 == PlaneIntersection::PositiveHalfspace;

		if (AnyOutside)
		{
			return ContainmentType::Disjoint;
		}

		if (AllInside)
		{
			return ContainmentType::Contains;
		}

		return ContainmentType::Intersects;
	}
} // namespace Math

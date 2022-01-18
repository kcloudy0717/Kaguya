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

inline void BoundingBox::GetCorners(
	Vec3f* FarBottomLeft,
	Vec3f* FarBottomRight,
	Vec3f* FarTopRight,
	Vec3f* FarTopLeft,
	Vec3f* NearBottomLeft,
	Vec3f* NearBottomRight,
	Vec3f* NearTopRight,
	Vec3f* NearTopLeft) const noexcept
{
	static constexpr size_t NumCorners			   = 8;
	static constexpr Vec3f	BoxOffsets[NumCorners] = {
		 Vec3f(-1.0f, -1.0f, +1.0f), // Far bottom left
		 Vec3f(+1.0f, -1.0f, +1.0f), // Far bottom right
		 Vec3f(+1.0f, +1.0f, +1.0f), // Far top right
		 Vec3f(-1.0f, +1.0f, +1.0f), // Far top left
		 Vec3f(-1.0f, -1.0f, -1.0f), // Near bottom left
		 Vec3f(+1.0f, -1.0f, -1.0f), // Near bottom right
		 Vec3f(+1.0f, +1.0f, -1.0f), // Near top right
		 Vec3f(-1.0f, +1.0f, -1.0f)	 // Near top left
	};

#define GET_CORNER(Arg, Index)                           \
	do                                                   \
	{                                                    \
		if (Arg)                                         \
		{                                                \
			*Arg = Extents * BoxOffsets[Index] + Center; \
		}                                                \
	} while (false)

	GET_CORNER(FarBottomLeft, 0);
	GET_CORNER(FarBottomRight, 1);
	GET_CORNER(FarTopRight, 2);
	GET_CORNER(FarTopLeft, 3);

	GET_CORNER(NearBottomLeft, 4);
	GET_CORNER(NearBottomRight, 5);
	GET_CORNER(NearTopRight, 6);
	GET_CORNER(NearTopLeft, 7);

#undef GET_CORNER
}

inline bool BoundingBox::Intersects(const BoundingBox& Other) const noexcept
{
	// does the range A_min -> A_max and B_min -> B_max for each axis overlap?
	// if it does then a intersection occurs
	// Y
	// |			---------
	// |			|		|
	// |		----|----	|
	// |		|	|///|	|
	// |		|	----|----
	// |		|  bXmin|	bXmax
	// |		---------
	// |	aXmin     aXmax
	// ------------------------------X
	//
	// Lets take a simpler look
	//
	//  |		 |		|		 |
	//  ----------		----------
	// min  A   max	   min	B	max
	// if Amax < Bmin then no overlap, for a overlap, Amax >= Bmin
	//
	// swap A and B for other side:
	//
	//  |		 |		|		 |
	//  ----------		----------
	// min  B   max	   min	A	max
	// if Amin > Bmax then no overlap, for a overlap, Amin <= Bmax
	//
	// Test this for each axis and you'll get intersection

	Vec3f MinA = Center - Extents;
	Vec3f MaxA = Center + Extents;

	Vec3f MinB = Other.Center - Other.Extents;
	Vec3f MaxB = Other.Center + Other.Extents;

	bool x = MaxA.x >= MinB.x && MinA.x <= MaxB.x; // Overlap on x-axis?
	bool y = MaxA.y >= MinB.y && MinA.y <= MaxB.y; // Overlap on y-axis?
	bool z = MaxA.z >= MinB.z && MinA.z <= MaxB.z; // Overlap on z-axis?

	// All axis needs to overlap for a collision
	return x && y && z;
}

inline PlaneIntersection BoundingBox::Intersects(const Plane& Plane) const noexcept
{
	// Compute signed distance from plane to box center
	float sd = dot(Center, Plane.Normal) - Plane.Offset;

	// Compute the projection interval radius of b onto L(t) = b.Center + t * p.Normal
	// Projection radii r_i of the 8 bounding box vertices
	// r_i = dot((V_i - C), n)
	// r_i = dot((C +- e0*u0 +- e1*u1 +- e2*u2 - C), n)
	// Cancel C and distribute dot product
	// r_i = +-(dot(e0*u0, n)) +-(dot(e1*u1, n)) +-(dot(e2*u2, n))
	// We take the maximum position radius by taking the absolute value of the terms, we assume Extents to be positive
	// r = e0*|dot(u0, n)| + e1*|dot(u1, n)| + e2*|dot(u2, n)|
	// When the separating axis vector Normal is not a unit vector, we need to divide the radii by the length(Normal)
	// u0,u1,u2 are the local axes of the box, which is = [(1,0,0), (0,1,0), (0,0,1)] respectively for axis aligned bb
	float r = dot(Extents, abs(Plane.Normal));

	if (sd > r)
	{
		return PlaneIntersection::PositiveHalfspace;
	}
	if (sd < -r)
	{
		return PlaneIntersection::NegativeHalfspace;
	}
	return PlaneIntersection::Intersecting;
}

inline void BoundingBox::Transform(DirectX::XMFLOAT4X4 Matrix, BoundingBox& BoundingBox) const noexcept
{
	BoundingBox.Center	= Vec3f(Matrix(3, 0), Matrix(3, 1), Matrix(3, 2));
	BoundingBox.Extents = Vec3f(0.0f, 0.0f, 0.0f);
	for (size_t i = 0; i < 3; ++i)
	{
		for (size_t j = 0; j < 3; ++j)
		{
			BoundingBox.Center[i] += Matrix(i, j) * Center[j];
			BoundingBox.Extents[i] += abs(Matrix(i, j)) * Extents[j];
		}
	}
}

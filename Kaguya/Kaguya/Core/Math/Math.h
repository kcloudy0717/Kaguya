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

#include "Vector2.h"
#include "Vector3.h"

// https://github.com/mmp/pbrt-v4/blob/master/src/pbrt/util/vecmath.h
// OctahedralVector Definition
class OctahedralVector
{
public:
	// OctahedralVector Public Methods
	OctahedralVector() noexcept = default;
	explicit OctahedralVector(Vector3f v) noexcept
	{
		v /= std::abs(v.x) + std::abs(v.y) + std::abs(v.z);
		if (v.z >= 0.0f)
		{
			x = Encode(v.x);
			y = Encode(v.y);
		}
		else
		{
			// Encode octahedral vector with $z < 0$
			x = Encode((1.0f - std::abs(v.y)) * SignNotZero(v.x));
			y = Encode((1.0f - std::abs(v.x)) * SignNotZero(v.y));
		}
	}

	explicit operator Vector3f() const noexcept
	{
		Vector3f v;
		v.x = -1.0f + 2.0f * (x / 65535.0f);
		v.y = -1.0f + 2.0f * (y / 65535.0f);
		v.z = 1.0f - (std::abs(v.x) + std::abs(v.y));
		// Reparameterize directions in the $z<0$ portion of the octahedron
		if (v.z < 0.0f)
		{
			float xo = v.x;
			v.x		 = (1.0f - std::abs(v.y)) * SignNotZero(xo);
			v.y		 = (1.0f - std::abs(xo)) * SignNotZero(v.y);
		}

		return normalize(v);
	}

private:
	// OctahedralVector Private Methods
	static uint16_t Encode(float f) noexcept
	{
		return static_cast<uint16_t>(std::round(std::clamp((f + 1.0f) / 2.0f, 0.0f, 1.0f) * 65535.0f));
	}

	static float SignNotZero(float v) noexcept { return (v < 0.0f) ? -1.0f : 1.0f; }

	// OctahedralVector Private Members
	uint16_t x, y;
};

struct Ray
{
	[[nodiscard]] bool Step(float StepSize = 0.1f) noexcept
	{
		o += d * StepSize;

		TMin += StepSize;

		return TMin < TMax;
	}

	Vector3f o;
	float	 TMin = 0.0f;
	Vector3f d;
	float	 TMax = 8.0f;
};

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

// ax + by + cz = d where d = dot(n, P)
struct Plane
{
	Plane() noexcept = default;
	explicit Plane(Vector3f a, Vector3f b, Vector3f c) noexcept;

	void Normalize() noexcept;

	Vector3f Normal; // (a,b,c)
	float	 Offset; // d
};

#define FRUSTUM_PLANE_LEFT	 0
#define FRUSTUM_PLANE_RIGHT	 1
#define FRUSTUM_PLANE_BOTTOM 2
#define FRUSTUM_PLANE_TOP	 3
#define FRUSTUM_PLANE_NEAR	 4
#define FRUSTUM_PLANE_FAR	 5

struct BoundingBox
{
	BoundingBox() noexcept = default;
	explicit BoundingBox(Vector3f Center, Vector3f Extents) noexcept;

	[[nodiscard]] std::array<Vector3f, 8> GetCorners() const noexcept;

	[[nodiscard]] bool				Intersects(const BoundingBox& Other) const noexcept;
	[[nodiscard]] PlaneIntersection Intersects(const Plane& Plane) const noexcept;

	void Transform(DirectX::XMFLOAT4X4 Matrix, BoundingBox& BoundingBox) const noexcept;

	Vector3f Center	 = { 0.0f, 0.0f, 0.0f };
	Vector3f Extents = { 1.0f, 1.0f, 1.0f };
};

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

#pragma once
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <cmath>
#include <limits>
#include <compare>
#include <algorithm>

// Returns radians
constexpr float operator"" _Deg(long double Degrees)
{
	return DirectX::XMConvertToRadians(static_cast<float>(Degrees));
}

// Returns degrees
constexpr float operator"" _Rad(long double Radians)
{
	return DirectX::XMConvertToDegrees(static_cast<float>(Radians));
}

template<typename T>
struct Vector2
{
	constexpr Vector2() noexcept { x = y = static_cast<T>(0); }

	constexpr Vector2(T v) noexcept
		: x(v)
		, y(v)
	{
	}

	constexpr Vector2(T x, T y) noexcept
		: x(x)
		, y(y)
	{
	}

	auto operator<=>(const Vector2&) const = default;

	T operator[](int i) const
	{
		assert(i < 2);
		return reinterpret_cast<const T*>(this)[i];
	}

	T& operator[](int i)
	{
		assert(i < 2);
		return reinterpret_cast<T*>(this)[i];
	}

	Vector2 operator-() const noexcept { return Vector2(-x, -y); }

	Vector2 operator+(const Vector2& v) const noexcept { return Vector2(x + v.x, y + v.y); }

	Vector2& operator+=(const Vector2& v) noexcept
	{
		x += v.x;
		y += v.y;
		return *this;
	}

	Vector2 operator-(const Vector2& v) const noexcept { return Vector2(x - v.x, y - v.y); }

	Vector2& operator-=(const Vector2& v) noexcept
	{
		x -= v.x;
		y -= v.y;
		return *this;
	}

	Vector2 operator*(const Vector2& v) const noexcept { return Vector2(x * v.x, y * v.y); }

	Vector2& operator*=(const Vector2& v) noexcept
	{
		x *= v.x;
		y *= v.y;
		return *this;
	}

	Vector2 operator/(T s) const noexcept
	{
		float inv = static_cast<T>(1) / s;
		return Vector2<T>(x * inv, y * inv);
	}

	Vector2& operator/=(T s) noexcept
	{
		float inv = static_cast<T>(1) / s;
		x *= inv;
		y *= inv;
		return *this;
	}

	T x, y;
};

template<typename T>
[[nodiscard]] Vector2<T> operator*(T s, const Vector2<T>& v) noexcept
{
	return v * s;
}

template<typename T>
[[nodiscard]] T length(const Vector2<T>& v) noexcept
{
	return std::hypot(v.x, v.y);
}

template<typename T>
[[nodiscard]] Vector2<T> abs(const Vector2<T>& v) noexcept
{
	return Vector2<T>(std::abs(v.x), std::abs(v.y));
}

template<typename T>
[[nodiscard]] T dot(const Vector2<T>& v1, const Vector2<T>& v2) noexcept
{
	return v1.x * v2.x + v1.y * v2.y;
}

template<typename T>
[[nodiscard]] Vector2<T> normalize(const Vector2<T>& v) noexcept
{
	return v / length(v);
}

template<typename T>
[[nodiscard]] bool isnan(const Vector2<T>& v) noexcept
{
	return std::isnan(v.x) || std::isnan(v.y);
}

template<typename T>
[[nodiscard]] Vector2<T> clamp(const Vector2<T>& v, T min, T max) noexcept
{
	return Vector2(std::clamp(v.x, min, max), std::clamp(v.y, min, max));
}

using Vector2i = Vector2<int>;
using Vector2f = Vector2<float>;
using Vector2d = Vector2<double>;

template<typename T>
struct Vector3
{
	constexpr Vector3() noexcept { x = y = z = static_cast<T>(0); }

	constexpr Vector3(T v) noexcept
		: x(v)
		, y(v)
		, z(v)
	{
	}

	constexpr Vector3(T x, T y, T z) noexcept
		: x(x)
		, y(y)
		, z(z)
	{
	}

	auto operator<=>(const Vector3&) const = default;

	T operator[](int i) const
	{
		assert(i < 3);
		return reinterpret_cast<const T*>(this)[i];
	}

	T& operator[](int i)
	{
		assert(i < 3);
		return reinterpret_cast<T*>(this)[i];
	}

	Vector3 operator-() const noexcept { return Vector3(-x, -y, -z); }

	Vector3 operator+(const Vector3& v) const noexcept { return Vector3(x + v.x, y + v.y, z + v.z); }

	Vector3& operator+=(const Vector3& v) noexcept
	{
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}

	Vector3 operator-(const Vector3& v) const noexcept { return Vector3(x - v.x, y - v.y, z - v.z); }

	Vector3& operator-=(const Vector3& v) noexcept
	{
		x -= v.x;
		y -= v.y;
		z -= v.z;
		return *this;
	}

	Vector3 operator*(const Vector3& v) const noexcept { return Vector3(x * v.x, y * v.y, z * v.z); }

	Vector3& operator*=(const Vector3& v) noexcept
	{
		x *= v.x;
		y *= v.y;
		z *= v.z;
		return *this;
	}

	Vector3 operator/(T s) const noexcept
	{
		float inv = static_cast<T>(1) / s;
		return Vector3<T>(x * inv, y * inv, z * inv);
	}

	Vector3& operator/=(T s) noexcept
	{
		float inv = static_cast<T>(1) / s;
		x *= inv;
		y *= inv;
		z *= inv;
		return *this;
	}

	T x, y, z;
};

template<typename T>
[[nodiscard]] Vector3<T> operator*(T s, const Vector3<T>& v) noexcept
{
	return v * s;
}

template<typename T>
[[nodiscard]] T length(const Vector3<T>& v) noexcept
{
	return std::hypot(v.x, v.y, v.z);
}

template<typename T>
[[nodiscard]] Vector3<T> abs(const Vector3<T>& v) noexcept
{
	return Vector3<T>(std::abs(v.x), std::abs(v.y), std::abs(v.z));
}

template<typename T>
[[nodiscard]] T dot(const Vector3<T>& v1, const Vector3<T>& v2) noexcept
{
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

template<typename T>
[[nodiscard]] Vector3<T> normalize(const Vector3<T>& v) noexcept
{
	return v / length(v);
}

template<typename T>
[[nodiscard]] bool isnan(const Vector3<T>& v) noexcept
{
	return std::isnan(v.x) || std::isnan(v.y) || std::isnan(v.z);
}

template<typename T>
[[nodiscard]] Vector3<T> cross(const Vector3<T>& v1, const Vector3<T>& v2) noexcept
{
	float v1x = v1.x, v1y = v1.y, v1z = v1.z;
	float v2x = v2.x, v2y = v2.y, v2z = v2.z;
	return Vector3<T>((v1y * v2z) - (v1z * v2y), (v1z * v2x) - (v1x * v2z), (v1x * v2y) - (v1y * v2x));
}

template<typename T>
[[nodiscard]] Vector3<T> faceforward(const Vector3<T>& n, const Vector3<T>& v) noexcept
{
	return (dot(n, v) < 0.0f) ? -n : n;
}

using Vector3i = Vector3<int>;
using Vector3f = Vector3<float>;
using Vector3d = Vector3<double>;

[[nodiscard]] constexpr float lerp(float x, float y, float s)
{
	return x + s * (y - x);
}

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

// ax + by + cz = d where d = dot(n, P)
struct Plane
{
	Plane() noexcept = default;
	Plane(Vector3f a, Vector3f b, Vector3f c)
	{
		Normal = normalize(cross(b - a, c - a));
		Offset = dot(Normal, a);
	}

	void Normalize()
	{
		float reciprocalMag = 1.0f / length(Normal);
		Normal *= reciprocalMag;
		Offset *= reciprocalMag;
	}

	Vector3f Normal; // (a,b,c)
	float	 Offset; // d
};

#define FRUSTUM_PLANE_LEFT	 0
#define FRUSTUM_PLANE_RIGHT	 1
#define FRUSTUM_PLANE_BOTTOM 2
#define FRUSTUM_PLANE_TOP	 3
#define FRUSTUM_PLANE_NEAR	 4
#define FRUSTUM_PLANE_FAR	 5

// All frustum planes are facing inwards of the frustum, so if any object
// is in the negative halfspace, then is not visible
struct Frustum
{
	Frustum() noexcept = default;
	explicit Frustum(const DirectX::XMFLOAT4X4& Matrix)
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

		Left.Normalize();
		Right.Normalize();
		Bottom.Normalize();
		Top.Normalize();
		Near.Normalize();
		Far.Normalize();
	}

	Plane Left;	  // -x
	Plane Right;  // +x
	Plane Bottom; // -y
	Plane Top;	  // +y
	Plane Near;	  // -z
	Plane Far;	  // +z
};

struct BoundingBox
{
	BoundingBox() noexcept
		: Center(0.0f, 0.0f, 0.0f)
		, Extents(1.0f, 1.0f, 1.0f)
	{
	}

	explicit BoundingBox(Vector3f Center, Vector3f Extents)
		: Center(Center)
		, Extents(Extents)
	{
	}

	std::array<Vector3f, 8> GetCorners() const
	{
		static constexpr size_t	  NumCorners			 = 8;
		static constexpr Vector3f BoxOffsets[NumCorners] = {
			Vector3f(-1.0f, -1.0f, +1.0f), // Far bottom left
			Vector3f(+1.0f, -1.0f, +1.0f), // Far bottom right
			Vector3f(+1.0f, +1.0f, +1.0f), // Far top right
			Vector3f(-1.0f, +1.0f, +1.0f), // Far top left
			Vector3f(-1.0f, -1.0f, -1.0f), // Near bottom left
			Vector3f(+1.0f, -1.0f, -1.0f), // Near bottom right
			Vector3f(+1.0f, +1.0f, -1.0f), // Near top right
			Vector3f(-1.0f, +1.0f, -1.0f)  // Near top left
		};

		std::array<Vector3f, 8> Corners = {};
		for (size_t i = 0; i < NumCorners; ++i)
		{
			Corners[i] = Extents * BoxOffsets[i] + Center;
		}
		return Corners;
	}

	bool Intersects(const BoundingBox& Other) const
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

		Vector3f MinA = Center - Extents;
		Vector3f MaxA = Center + Extents;

		Vector3f MinB = Other.Center - Other.Extents;
		Vector3f MaxB = Other.Center + Other.Extents;

		bool x = MaxA.x >= MinB.x && MinA.x <= MaxB.x; // Overlap on x-axis?
		bool y = MaxA.y >= MinB.y && MinA.y <= MaxB.y; // Overlap on y-axis?
		bool z = MaxA.z >= MinB.z && MinA.z <= MaxB.z; // Overlap on z-axis?

		// All axis needs to overlap for a collision
		return x && y && z;
	}

	void Transform(DirectX::XMFLOAT4X4 Matrix, BoundingBox& BoundingBox) const
	{
		BoundingBox.Center	= Vector3f(Matrix(3, 0), Matrix(3, 1), Matrix(3, 2));
		BoundingBox.Extents = Vector3f(0.0f, 0.0f, 0.0f);
		for (int i = 0; i < 3; ++i)
		{
			for (int j = 0; j < 3; ++j)
			{
				BoundingBox.Center[i] += Matrix(i, j) * Center[j];
				BoundingBox.Extents[i] += abs(Matrix(i, j)) * Extents[j];
			}
		}
	}

	Vector3f Center;
	Vector3f Extents;
};

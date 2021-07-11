#pragma once
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <cmath>
#include <limits>

// Returns radians
constexpr inline float operator"" _Deg(long double Degrees)
{
	return DirectX::XMConvertToRadians(static_cast<float>(Degrees));
}

// Returns degrees
constexpr inline float operator"" _Rad(long double Radians)
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

	T operator[](int i) const { return e[i]; }

	T& operator[](int i) { return e[i]; }

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

	union
	{
		T e[2];
		struct
		{
			T x, y;
		};
	};
};

template<typename T>
[[nodiscard]] inline Vector2<T> operator*(T s, const Vector2<T>& v) noexcept
{
	return v * s;
}

template<typename T>
[[nodiscard]] inline T length(const Vector2<T>& v) noexcept
{
	return std::sqrt(v.x * v.x + v.y * v.y);
}

template<typename T>
[[nodiscard]] inline Vector2<T> abs(const Vector2<T>& v) noexcept
{
	return Vector2<T>(std::abs(v.x), std::abs(v.y));
}

template<typename T>
[[nodiscard]] inline T dot(const Vector2<T>& v1, const Vector2<T>& v2) noexcept
{
	return v1.x * v2.x + v1.y * v2.y;
}

template<typename T>
[[nodiscard]] inline Vector2<T> normalize(const Vector2<T>& v) noexcept
{
	return v / length(v);
}

template<typename T>
[[nodiscard]] inline bool isnan(const Vector2<T>& v) noexcept
{
	return std::isnan(v.x) || std::isnan(v.y);
}

template<typename T>
[[nodiscard]] inline Vector2<T> clamp(const Vector2<T>& v, T min, T max) noexcept
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

	T operator[](int i) const { return e[i]; }

	T& operator[](int i) { return e[i]; }

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

	union
	{
		T e[3];
		struct
		{
			T x, y, z;
		};
	};
};

template<typename T>
[[nodiscard]] inline Vector3<T> operator*(T s, const Vector3<T>& v) noexcept
{
	return v * s;
}

template<typename T>
[[nodiscard]] inline T length(const Vector3<T>& v) noexcept
{
	return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

template<typename T>
[[nodiscard]] inline Vector3<T> abs(const Vector3<T>& v) noexcept
{
	return Vector3<T>(std::abs(v.x), std::abs(v.y), std::abs(v.z));
}

template<typename T>
[[nodiscard]] inline T dot(const Vector3<T>& v1, const Vector3<T>& v2) noexcept
{
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

template<typename T>
[[nodiscard]] inline Vector3<T> normalize(const Vector3<T>& v) noexcept
{
	return v / length(v);
}

template<typename T>
[[nodiscard]] inline bool isnan(const Vector3<T>& v) noexcept
{
	return std::isnan(v.x) || std::isnan(v.y) || std::isnan(v.z);
}

template<typename T>
[[nodiscard]] inline Vector3<T> cross(const Vector3<T>& v1, const Vector3<T>& v2) noexcept
{
	float v1x = v1.x, v1y = v1.y, v1z = v1.z;
	float v2x = v2.x, v2y = v2.y, v2z = v2.z;
	return Vector3<T>((v1y * v2z) - (v1z * v2y), (v1z * v2x) - (v1x * v2z), (v1x * v2y) - (v1y * v2x));
}

template<typename T>
[[nodiscard]] inline Vector3<T> faceforward(const Vector3<T>& n, const Vector3<T>& v) noexcept
{
	return (dot(n, v) < 0.0f) ? -n : n;
}

using Vector3i = Vector3<int>;
using Vector3f = Vector3<float>;
using Vector3d = Vector3<double>;

[[nodiscard]] inline constexpr float lerp(float x, float y, float s)
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

inline static constexpr size_t	 NumCorners				  = 8;
inline static constexpr Vector3f g_BoxOffsets[NumCorners] = {
	Vector3f(-1.0f, -1.0f, +1.0f), // Far bottom left
	Vector3f(+1.0f, -1.0f, +1.0f), // Far bottom right
	Vector3f(+1.0f, +1.0f, +1.0f), // Far top right
	Vector3f(-1.0f, +1.0f, +1.0f), // Far top left
	Vector3f(-1.0f, -1.0f, -1.0f), // Near bottom left
	Vector3f(+1.0f, -1.0f, -1.0f), // Near bottom right
	Vector3f(+1.0f, +1.0f, -1.0f), // Near top right
	Vector3f(-1.0f, +1.0f, -1.0f)  // Near top left
};

struct BoundingBox
{
	BoundingBox() noexcept
		: Center(0.0f, 0.0f, 0.0f)
		, Extents(1.0f, 1.0f, 1.0f)
	{
	}

	BoundingBox(Vector3f Center, Vector3f Extents)
		: Center(Center)
		, Extents(Extents)
	{
	}

	std::array<Vector3f, 8> GetCorners() const
	{
		std::array<Vector3f, 8> corners = {};

		for (size_t i = 0; i < NumCorners; ++i)
		{
			corners[i] = Extents * g_BoxOffsets[i] + Center;
		}

		return corners;
	}

	bool Intersects(const BoundingBox& box) const
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

		Vector3f minA = Center - Extents;
		Vector3f maxA = Center + Extents;

		Vector3f minB = box.Center - box.Extents;
		Vector3f maxB = box.Center + box.Extents;

		bool x = maxA.x >= minB.x && minA.x <= maxB.x; // Overlap on x-axis?
		bool y = maxA.y >= minB.y && minA.y <= maxB.y; // Overlap on y-axis?
		bool z = maxA.z >= minB.z && minA.z <= maxB.z; // Overlap on z-axis?

		// All axis needs to overlap for a collision
		return x && y && z;
	}

	Vector3f Center;
	Vector3f Extents;
};

#pragma once
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <cmath>
#include <limits>
#include <compare>
#include <algorithm>

template<typename T>
struct Vec2
{
	constexpr Vec2() noexcept
		: x(static_cast<T>(0))
		, y(static_cast<T>(0))
	{
	}

	constexpr Vec2(T x, T y) noexcept
		: x(x)
		, y(y)
	{
	}

	constexpr Vec2(T v) noexcept
		: Vec2(v, v)
	{
	}

	T*		 data() noexcept { return reinterpret_cast<T*>(this); }
	const T* data() const noexcept { return reinterpret_cast<const T*>(this); }

	T operator[](int i) const noexcept
	{
		assert(i < 2);
		return this->*Array[i];
	}

	T& operator[](int i) noexcept
	{
		assert(i < 2);
		return this->*Array[i];
	}

	Vec2 operator-() const noexcept
	{
		return Vec2(-x, -y);
	}

	Vec2 operator+(const Vec2& v) const noexcept
	{
		return Vec2(x + v.x, y + v.y);
	}

	Vec2& operator+=(const Vec2& v) noexcept
	{
		x += v.x;
		y += v.y;
		return *this;
	}

	Vec2 operator-(const Vec2& v) const noexcept
	{
		return Vec2(x - v.x, y - v.y);
	}

	Vec2& operator-=(const Vec2& v) noexcept
	{
		x -= v.x;
		y -= v.y;
		return *this;
	}

	Vec2 operator*(const Vec2& v) const noexcept
	{
		return Vec2(x * v.x, y * v.y);
	}

	Vec2& operator*=(const Vec2& v) noexcept
	{
		x *= v.x;
		y *= v.y;
		return *this;
	}

	Vec2 operator/(T s) const noexcept
	{
		float inv = static_cast<T>(1) / s;
		return Vec2<T>(x * inv, y * inv);
	}

	Vec2& operator/=(T s) noexcept
	{
		float inv = static_cast<T>(1) / s;
		x *= inv;
		y *= inv;
		return *this;
	}

	T x, y;

private:
	static constexpr T Vec2::*Array[]{ &Vec2::x, &Vec2::y };
};

template<typename T>
[[nodiscard]] T length(const Vec2<T>& v) noexcept
{
	return std::hypot(v.x, v.y);
}

template<typename T>
[[nodiscard]] Vec2<T> abs(const Vec2<T>& v) noexcept
{
	return Vec2<T>(std::abs(v.x), std::abs(v.y));
}

template<typename T>
[[nodiscard]] T dot(const Vec2<T>& v1, const Vec2<T>& v2) noexcept
{
	return v1.x * v2.x + v1.y * v2.y;
}

template<typename T>
[[nodiscard]] Vec2<T> normalize(const Vec2<T>& v) noexcept
{
	return v / length(v);
}

template<typename T>
[[nodiscard]] bool isnan(const Vec2<T>& v) noexcept
{
	return std::isnan(v.x) || std::isnan(v.y);
}

template<typename T>
[[nodiscard]] Vec2<T> clamp(const Vec2<T>& v, T min, T max) noexcept
{
	return Vec2(std::clamp(v.x, min, max), std::clamp(v.y, min, max));
}

template<typename T>
struct Vec3
{
	constexpr Vec3() noexcept
		: x(static_cast<T>(0))
		, y(static_cast<T>(0))
		, z(static_cast<T>(0))
	{
	}

	constexpr Vec3(T x, T y, T z) noexcept
		: x(x)
		, y(y)
		, z(z)
	{
	}

	constexpr Vec3(T v) noexcept
		: Vec3(v, v, v)
	{
	}

	T*		 data() noexcept { return reinterpret_cast<T*>(this); }
	const T* data() const noexcept { return reinterpret_cast<const T*>(this); }

	constexpr T operator[](int i) const noexcept
	{
		assert(i < 3);
		return this->*Array[i];
	}

	constexpr T& operator[](int i) noexcept
	{
		assert(i < 3);
		return this->*Array[i];
	}

	Vec3 operator-() const noexcept
	{
		return Vec3(-x, -y, -z);
	}

	Vec3 operator+(const Vec3& v) const noexcept
	{
		return Vec3(x + v.x, y + v.y, z + v.z);
	}

	Vec3& operator+=(const Vec3& v) noexcept
	{
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}

	Vec3 operator-(const Vec3& v) const noexcept
	{
		return Vec3(x - v.x, y - v.y, z - v.z);
	}

	Vec3& operator-=(const Vec3& v) noexcept
	{
		x -= v.x;
		y -= v.y;
		z -= v.z;
		return *this;
	}

	Vec3 operator*(const Vec3& v) const noexcept
	{
		return Vec3(x * v.x, y * v.y, z * v.z);
	}

	Vec3& operator*=(const Vec3& v) noexcept
	{
		x *= v.x;
		y *= v.y;
		z *= v.z;
		return *this;
	}

	Vec3 operator/(T s) const noexcept
	{
		float inv = static_cast<T>(1) / s;
		return Vec3<T>(x * inv, y * inv, z * inv);
	}

	Vec3& operator/=(T s) noexcept
	{
		float inv = static_cast<T>(1) / s;
		x *= inv;
		y *= inv;
		z *= inv;
		return *this;
	}

	T x, y, z;

private:
	static constexpr T Vec3::*Array[]{ &Vec3::x, &Vec3::y, &Vec3::z };
};

template<typename T>
[[nodiscard]] T length(const Vec3<T>& v) noexcept
{
	return std::hypot(v.x, v.y, v.z);
}

template<typename T>
[[nodiscard]] Vec3<T> abs(const Vec3<T>& v) noexcept
{
	return Vec3<T>(std::abs(v.x), std::abs(v.y), std::abs(v.z));
}

template<typename T>
[[nodiscard]] T dot(const Vec3<T>& v1, const Vec3<T>& v2) noexcept
{
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

template<typename T>
[[nodiscard]] Vec3<T> normalize(const Vec3<T>& v) noexcept
{
	return v / length(v);
}

template<typename T>
[[nodiscard]] bool isnan(const Vec3<T>& v) noexcept
{
	return std::isnan(v.x) || std::isnan(v.y) || std::isnan(v.z);
}

template<typename T>
[[nodiscard]] Vec3<T> cross(const Vec3<T>& v1, const Vec3<T>& v2) noexcept
{
	float v1x = v1.x, v1y = v1.y, v1z = v1.z;
	float v2x = v2.x, v2y = v2.y, v2z = v2.z;
	return Vec3<T>((v1y * v2z) - (v1z * v2y), (v1z * v2x) - (v1x * v2z), (v1x * v2y) - (v1y * v2x));
}

template<typename T>
[[nodiscard]] Vec3<T> faceforward(const Vec3<T>& n, const Vec3<T>& v) noexcept
{
	return (dot(n, v) < 0.0f) ? -n : n;
}

using Vec2i = Vec2<int>;
using Vec2f = Vec2<float>;
using Vec2d = Vec2<double>;

using Vec3i = Vec3<int>;
using Vec3f = Vec3<float>;
using Vec3d = Vec3<double>;

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

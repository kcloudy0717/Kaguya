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

constexpr inline std::size_t operator"" _KiB(std::size_t X)
{
	return X * 1024;
}

constexpr inline std::size_t operator"" _MiB(std::size_t X)
{
	return X * 1024 * 1024;
}

constexpr inline std::size_t operator"" _GiB(std::size_t X)
{
	return X * 1024 * 1024 * 1024;
}

inline std::size_t ToKiB(std::size_t Byte)
{
	return Byte / 1024;
}

inline std::size_t ToMiB(std::size_t Byte)
{
	return Byte / 1024 / 1024;
}

inline std::size_t ToGiB(std::size_t Byte)
{
	return Byte / 1024 / 1024 / 1024;
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

	Vector2 operator*(T s) const noexcept { return Vector2(x * s, y * s); }

	Vector2& operator*=(T s) noexcept
	{
		x *= s;
		y *= s;
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

	Vector3 operator*(T s) const noexcept { return Vector3(x * s, y * s, z * s); }

	Vector3& operator*=(T s) noexcept
	{
		x *= s;
		y *= s;
		z *= s;
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

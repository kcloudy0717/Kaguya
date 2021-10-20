#pragma once

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

	T*		 data() noexcept { return reinterpret_cast<T*>(this); }
	const T* data() const noexcept { return reinterpret_cast<const T*>(this); }

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

#pragma once

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

	T*		 data() noexcept { return reinterpret_cast<T*>(this); }
	const T* data() const noexcept { return reinterpret_cast<const T*>(this); }

	auto operator<=>(const Vector2&) const = default;

	T operator[](int i) const noexcept
	{
		assert(i < 2);
		return reinterpret_cast<const T*>(this)[i];
	}

	T& operator[](int i) noexcept
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

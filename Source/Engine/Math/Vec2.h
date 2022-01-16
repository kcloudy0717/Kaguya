#pragma once
#include <cassert>
#include <cmath>
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

using Vec2i = Vec2<int>;
using Vec2f = Vec2<float>;
using Vec2d = Vec2<double>;

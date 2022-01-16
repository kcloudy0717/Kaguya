#pragma once
#include <cassert>
#include <cmath>
#include <algorithm>

template<typename T>
struct Vec4
{
	constexpr Vec4() noexcept
		: x(static_cast<T>(0))
		, y(static_cast<T>(0))
		, z(static_cast<T>(0))
	{
	}

	constexpr Vec4(T x, T y, T z, T w) noexcept
		: x(x)
		, y(y)
		, z(z)
		, w(w)
	{
	}

	constexpr Vec4(T v) noexcept
		: Vec4(v, v, v, v)
	{
	}

	T*		 data() noexcept { return reinterpret_cast<T*>(this); }
	const T* data() const noexcept { return reinterpret_cast<const T*>(this); }

	constexpr T operator[](int i) const noexcept
	{
		assert(i < 4);
		return this->*Array[i];
	}

	constexpr T& operator[](int i) noexcept
	{
		assert(i < 4);
		return this->*Array[i];
	}

	Vec4 operator-() const noexcept
	{
		return Vec4(-x, -y, -z, -w);
	}

	Vec4 operator+(const Vec4& v) const noexcept
	{
		return Vec4(x + v.x, y + v.y, z + v.z, w + v.w);
	}

	Vec4& operator+=(const Vec4& v) noexcept
	{
		x += v.x;
		y += v.y;
		z += v.z;
		w += v.w;
		return *this;
	}

	Vec4 operator-(const Vec4& v) const noexcept
	{
		return Vec4(x - v.x, y - v.y, z - v.z, w - v.w);
	}

	Vec4& operator-=(const Vec4& v) noexcept
	{
		x -= v.x;
		y -= v.y;
		z -= v.z;
		w -= v.w;
		return *this;
	}

	Vec4 operator*(const Vec4& v) const noexcept
	{
		return Vec4(x * v.x, y * v.y, z * v.z, w * v.w);
	}

	Vec4& operator*=(const Vec4& v) noexcept
	{
		x *= v.x;
		y *= v.y;
		z *= v.z;
		w *= v.w;
		return *this;
	}

	Vec4 operator/(T s) const noexcept
	{
		float inv = static_cast<T>(1) / s;
		return Vec4(x * inv, y * inv, z * inv, w * inv);
	}

	Vec4& operator/=(T s) noexcept
	{
		float inv = static_cast<T>(1) / s;
		x *= inv;
		y *= inv;
		z *= inv;
		w *= inv;
		return *this;
	}

	T x, y, z, w;

private:
	static constexpr T Vec4::*Array[]{ &Vec4::x, &Vec4::y, &Vec4::z, &Vec4::w };
};

using Vec4i = Vec4<int>;
using Vec4f = Vec4<float>;
using Vec4d = Vec4<double>;

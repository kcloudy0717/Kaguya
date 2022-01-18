#pragma once
#include <cassert>
#include <cmath>
#include <algorithm>
#include "Intrinsics.h"

namespace xvm
{
	struct Vec3
	{
		Vec3() noexcept
			: vec(_mm_setzero_ps())
		{
		}
		Vec3(__m128 a) noexcept
			: vec(a)
		{
		}
		Vec3(float x, float y, float z) noexcept
			: vec(_mm_set_ps(0.0f, z, y, x))
		{
		}
		Vec3(float v) noexcept
			: Vec3(v, v, v)
		{
		}

		float operator[](size_t Index) const noexcept
		{
			assert(Index < 3);
			return f32[Index];
		}

		float& operator[](size_t Index) noexcept
		{
			assert(Index < 3);
			return f32[Index];
		}

		union
		{
			__m128				  vec;
			float				  f32[4];
			xvm::ScalarSwizzle<0> x;
			xvm::ScalarSwizzle<1> y;
			xvm::ScalarSwizzle<2> z;
		};
	};

	Vec3 XVM_CALLCONV  operator-(Vec3 v);
	Vec3 XVM_CALLCONV  operator+(Vec3 v1, Vec3 v2);
	Vec3 XVM_CALLCONV  operator-(Vec3 v1, Vec3 v2);
	Vec3 XVM_CALLCONV  operator*(Vec3 v1, Vec3 v2);
	Vec3 XVM_CALLCONV  operator/(Vec3 v1, Vec3 v2);
	Vec3& XVM_CALLCONV operator+=(Vec3& v1, Vec3 v2);
	Vec3& XVM_CALLCONV operator-=(Vec3& v1, Vec3 v2);
	Vec3& XVM_CALLCONV operator*=(Vec3& v1, Vec3 v2);
	Vec3& XVM_CALLCONV operator/=(Vec3& v1, Vec3 v2);

	bool XVM_CALLCONV all(Vec3 v);
	bool XVM_CALLCONV any(Vec3 v);
	bool XVM_CALLCONV isfinite(Vec3 v);
	bool XVM_CALLCONV isinf(Vec3 v);
	bool XVM_CALLCONV isnan(Vec3 v);

	Vec3 XVM_CALLCONV abs(Vec3 v);
	Vec3 XVM_CALLCONV clamp(Vec3 v, Vec3 min, Vec3 max);
	Vec3 XVM_CALLCONV cross(Vec3 v1, Vec3 v2);
	Vec3 XVM_CALLCONV dot(Vec3 v1, Vec3 v2);
	Vec3 XVM_CALLCONV length(Vec3 v);
	Vec3 XVM_CALLCONV lerp(Vec3 v1, Vec3 v2, Vec3 s);
	Vec3 XVM_CALLCONV max(Vec3 v1, Vec3 v2);
	Vec3 XVM_CALLCONV min(Vec3 v1, Vec3 v2);
	Vec3 XVM_CALLCONV normalize(Vec3 v);
	Vec3 XVM_CALLCONV sqrt(Vec3 v);

	XVM_INLINE Vec3 XVM_CALLCONV operator-(Vec3 v)
	{
		return _mm_xor_ps(v.vec, xvm::XVMMaskNegative);
	}

	XVM_INLINE Vec3 XVM_CALLCONV operator+(Vec3 v1, Vec3 v2)
	{
		return _mm_add_ps(v1.vec, v2.vec);
	}

	XVM_INLINE Vec3 XVM_CALLCONV operator-(Vec3 v1, Vec3 v2)
	{
		return _mm_sub_ps(v1.vec, v2.vec);
	}

	XVM_INLINE Vec3 XVM_CALLCONV operator*(Vec3 v1, Vec3 v2)
	{
		return _mm_mul_ps(v1.vec, v2.vec);
	}

	XVM_INLINE Vec3 XVM_CALLCONV operator/(Vec3 v1, Vec3 v2)
	{
		return _mm_div_ps(v1.vec, v2.vec);
	}

	XVM_INLINE Vec3& XVM_CALLCONV operator+=(Vec3& v1, Vec3 v2)
	{
		v1 = v1 + v2;
		return v1;
	}

	XVM_INLINE Vec3& XVM_CALLCONV operator-=(Vec3& v1, Vec3 v2)
	{
		v1 = v1 - v2;
		return v1;
	}

	XVM_INLINE Vec3& XVM_CALLCONV operator*=(Vec3& v1, Vec3 v2)
	{
		v1 = v1 * v2;
		return v1;
	}

	XVM_INLINE Vec3& XVM_CALLCONV operator/=(Vec3& v1, Vec3 v2)
	{
		v1 = v1 / v2;
		return v1;
	}

	XVM_INLINE bool XVM_CALLCONV all(Vec3 v)
	{
		__m128 cmp	= _mm_cmpneq_ps(v.vec, _mm_setzero_ps()); // v != 0
		int	   mask = _mm_movemask_ps(cmp) & 0x00000007;	  // If all is non-zero, the mask is exactly 0x00000007
		return mask == 0x00000007;
	}

	XVM_INLINE bool XVM_CALLCONV any(Vec3 v)
	{
		__m128 cmp	= _mm_cmpneq_ps(v.vec, _mm_setzero_ps()); // v != 0
		int	   mask = _mm_movemask_ps(cmp) & 0x00000007;	  // If x or y or z are non-zero, the mask is non-zero
		return mask != 0;
	}

	XVM_INLINE bool XVM_CALLCONV isfinite(Vec3 v)
	{
		__m128 cmp = _mm_and_ps(v.vec, xvm::XVMMaskAbsoluteValue); // Mask off the sign bit
		cmp		   = _mm_cmpneq_ps(cmp, xvm::XVMInfinity);		   // v != infinity
		int mask   = _mm_movemask_ps(cmp) & 0x00000007;			   // If x, y, z is finite, the mask is exactly 0x00000007
		return mask == 0x00000007;
	}

	XVM_INLINE bool XVM_CALLCONV isinf(Vec3 v)
	{
		__m128 cmp = _mm_and_ps(v.vec, xvm::XVMMaskAbsoluteValue); // Mask off the sign bit then compare to infinity
		cmp		   = _mm_cmpeq_ps(cmp, xvm::XVMInfinity);		   // v == infinity
		int mask   = _mm_movemask_ps(cmp) & 0x00000007;			   // If x or y or z is infinity, the mask is non-zero
		return mask != 0;
	}

	XVM_INLINE bool XVM_CALLCONV isnan(Vec3 v)
	{
		__m128 dst	= _mm_cmpneq_ps(v.vec, v.vec);		 // v != v
		int	   mask = _mm_movemask_ps(dst) & 0x00000007; // If x or y or z are nan, the mask is non-zero
		return mask != 0;
	}

	XVM_INLINE Vec3 XVM_CALLCONV abs(Vec3 v)
	{
		// & the sign bit
		return _mm_and_ps(xvm::XVMMaskAbsoluteValue, v.vec);
	}

	XVM_INLINE Vec3 XVM_CALLCONV clamp(Vec3 v, Vec3 min, Vec3 max)
	{
		__m128 xmm0 = _mm_min_ps(v.vec, max.vec);
		__m128 xmm1 = _mm_max_ps(xmm0, min.vec);
		return xmm1;
	}

	XVM_INLINE Vec3 XVM_CALLCONV cross(Vec3 v1, Vec3 v2)
	{
		return xvm::_xvm_mm_cross_ps(v1.vec, v2.vec);
	}

	XVM_INLINE Vec3 XVM_CALLCONV dot(Vec3 v1, Vec3 v2)
	{
		__m128 dot	 = _mm_mul_ps(v1.vec, v2.vec);
		__m128 shuff = XVM_SWIZZLE_PS(dot, _MM_SHUFFLE(2, 2, 2, 1)); // { y, z, z, z }
		dot			 = _mm_add_ss(dot, shuff);						 // { x+y, ?, ?, ? }
		shuff		 = XVM_SWIZZLE_PS(dot, _MM_SHUFFLE(2, 2, 2, 2)); // { z, z, z, z }
		dot			 = _mm_add_ss(dot, shuff);						 // { x+y+z, ?, ?, ? }
		return XVM_SWIZZLE_PS(dot, _MM_SHUFFLE(0, 0, 0, 0));		 // splat fp0
	}

	XVM_INLINE Vec3 XVM_CALLCONV length(Vec3 v)
	{
		Vec3 v2 = dot(v, v);
		return _mm_sqrt_ps(v2.vec);
	}

	XVM_INLINE Vec3 XVM_CALLCONV lerp(Vec3 v1, Vec3 v2, Vec3 s)
	{
		// x + s(y-x)
		__m128 sub = _mm_sub_ps(v2.vec, v1.vec);
		__m128 mul = _mm_mul_ps(sub, s.vec);
		return _mm_add_ps(v1.vec, mul);
	}

	XVM_INLINE Vec3 XVM_CALLCONV max(Vec3 v1, Vec3 v2)
	{
		return _mm_max_ps(v1.vec, v2.vec);
	}

	XVM_INLINE Vec3 XVM_CALLCONV min(Vec3 v1, Vec3 v2)
	{
		return _mm_min_ps(v1.vec, v2.vec);
	}

	XVM_INLINE Vec3 XVM_CALLCONV normalize(Vec3 v)
	{
		Vec3 l = length(v);
		return _mm_div_ps(v.vec, l.vec);
	}

	XVM_INLINE Vec3 XVM_CALLCONV sqrt(Vec3 v)
	{
		return _mm_sqrt_ps(v.vec);
	}
} // namespace xvm

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

	constexpr T operator[](size_t i) const noexcept
	{
		assert(i < 3);
		return this->*Array[i];
	}

	constexpr T& operator[](size_t i) noexcept
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

	Vec3 operator-(const Vec3& v) const noexcept
	{
		return Vec3(x - v.x, y - v.y, z - v.z);
	}

	Vec3 operator*(const Vec3& v) const noexcept
	{
		return Vec3(x * v.x, y * v.y, z * v.z);
	}

	Vec3 operator/(T s) const noexcept
	{
		float inv = static_cast<T>(1) / s;
		return Vec3(x * inv, y * inv, z * inv);
	}

	Vec3& operator+=(const Vec3& v) noexcept
	{
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}

	Vec3& operator-=(const Vec3& v) noexcept
	{
		x -= v.x;
		y -= v.y;
		z -= v.z;
		return *this;
	}

	Vec3& operator*=(const Vec3& v) noexcept
	{
		x *= v.x;
		y *= v.y;
		z *= v.z;
		return *this;
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
	return dot(n, v) < 0.0f ? -n : n;
}

using Vec3i = Vec3<int>;
using Vec3f = Vec3<float>;
using Vec3d = Vec3<double>;

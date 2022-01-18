#pragma once
#include <cassert>
#include <cmath>
#include <algorithm>
#include "Intrinsics.h"

namespace xvm
{
	struct Vec4
	{
		Vec4() noexcept
			: vec(_mm_setzero_ps())
		{
		}
		Vec4(__m128 a) noexcept
			: vec(a)
		{
		}
		Vec4(float x, float y, float z, float w) noexcept
			: vec(_mm_set_ps(w, z, y, x))
		{
		}
		Vec4(float v) noexcept
			: Vec4(v, v, v, v)
		{
		}

		float operator[](size_t Index) const noexcept
		{
			assert(Index < 4);
			return f32[Index];
		}

		float& operator[](size_t Index) noexcept
		{
			assert(Index < 4);
			return f32[Index];
		}

		union
		{
			__m128				  vec;
			float				  f32[4];
			xvm::ScalarSwizzle<0> x;
			xvm::ScalarSwizzle<1> y;
			xvm::ScalarSwizzle<2> z;
			xvm::ScalarSwizzle<3> w;
		};
	};

	Vec4 XVM_CALLCONV  operator-(Vec4 v);
	Vec4 XVM_CALLCONV  operator+(Vec4 v1, Vec4 v2);
	Vec4 XVM_CALLCONV  operator-(Vec4 v1, Vec4 v2);
	Vec4 XVM_CALLCONV  operator*(Vec4 v1, Vec4 v2);
	Vec4 XVM_CALLCONV  operator/(Vec4 v1, Vec4 v2);
	Vec4& XVM_CALLCONV operator+=(Vec4& v1, Vec4 v2);
	Vec4& XVM_CALLCONV operator-=(Vec4& v1, Vec4 v2);
	Vec4& XVM_CALLCONV operator*=(Vec4& v1, Vec4 v2);
	Vec4& XVM_CALLCONV operator/=(Vec4& v1, Vec4 v2);

	bool XVM_CALLCONV all(Vec4 v);
	bool XVM_CALLCONV any(Vec4 v);
	bool XVM_CALLCONV isfinite(Vec4 v);
	bool XVM_CALLCONV isinf(Vec4 v);
	bool XVM_CALLCONV isnan(Vec4 v);

	Vec4 XVM_CALLCONV abs(Vec4 v);
	Vec4 XVM_CALLCONV clamp(Vec4 v, Vec4 min, Vec4 max);
	Vec4 XVM_CALLCONV dot(Vec4 v1, Vec4 v2);
	Vec4 XVM_CALLCONV length(Vec4 v);
	Vec4 XVM_CALLCONV lerp(Vec4 v1, Vec4 v2, Vec4 s);
	Vec4 XVM_CALLCONV max(Vec4 v1, Vec4 v2);
	Vec4 XVM_CALLCONV min(Vec4 v1, Vec4 v2);
	Vec4 XVM_CALLCONV normalize(Vec4 v);
	Vec4 XVM_CALLCONV sqrt(Vec4 v);

	XVM_INLINE Vec4 XVM_CALLCONV operator-(Vec4 v)
	{
		return _mm_xor_ps(v.vec, xvm::XVMMaskNegative);
	}

	XVM_INLINE Vec4 XVM_CALLCONV operator+(Vec4 v1, Vec4 v2)
	{
		return _mm_add_ps(v1.vec, v2.vec);
	}

	XVM_INLINE Vec4 XVM_CALLCONV operator-(Vec4 v1, Vec4 v2)
	{
		return _mm_sub_ps(v1.vec, v2.vec);
	}

	XVM_INLINE Vec4 XVM_CALLCONV operator*(Vec4 v1, Vec4 v2)
	{
		return _mm_mul_ps(v1.vec, v2.vec);
	}

	XVM_INLINE Vec4 XVM_CALLCONV operator/(Vec4 v1, Vec4 v2)
	{
		return _mm_div_ps(v1.vec, v2.vec);
	}

	XVM_INLINE Vec4& XVM_CALLCONV operator+=(Vec4& v1, Vec4 v2)
	{
		v1 = v1 + v2;
		return v1;
	}

	XVM_INLINE Vec4& XVM_CALLCONV operator-=(Vec4& v1, Vec4 v2)
	{
		v1 = v1 - v2;
		return v1;
	}

	XVM_INLINE Vec4& XVM_CALLCONV operator*=(Vec4& v1, Vec4 v2)
	{
		v1 = v1 * v2;
		return v1;
	}

	XVM_INLINE Vec4& XVM_CALLCONV operator/=(Vec4& v1, Vec4 v2)
	{
		v1 = v1 / v2;
		return v1;
	}

	XVM_INLINE bool XVM_CALLCONV all(Vec4 v)
	{
		__m128 cmp	= _mm_cmpneq_ps(v.vec, _mm_setzero_ps()); // v != 0
		int	   mask = _mm_movemask_ps(cmp) & 0x0000000F;	  // If all is non-zero, the mask is exactly 0x0000000F
		return mask == 0x0000000F;
	}

	XVM_INLINE bool XVM_CALLCONV any(Vec4 v)
	{
		__m128 cmp	= _mm_cmpneq_ps(v.vec, _mm_setzero_ps()); // v != 0
		int	   mask = _mm_movemask_ps(cmp) & 0x0000000F;	  // If x or y or z or w are non-zero, the mask is non-zero
		return mask != 0;
	}

	XVM_INLINE bool XVM_CALLCONV isfinite(Vec4 v)
	{
		__m128 cmp = _mm_and_ps(v.vec, xvm::XVMMaskAbsoluteValue); // Mask off the sign bit
		cmp		   = _mm_cmpneq_ps(cmp, xvm::XVMInfinity);		   // v != infinity
		int mask   = _mm_movemask_ps(cmp) & 0x0000000F;			   // If x, y, z, w is finite, the mask is exactly 0x0000000F
		return mask == 0x0000000F;
	}

	XVM_INLINE bool XVM_CALLCONV isinf(Vec4 v)
	{
		__m128 cmp = _mm_and_ps(v.vec, xvm::XVMMaskAbsoluteValue); // Mask off the sign bit then compare to infinity
		cmp		   = _mm_cmpeq_ps(cmp, xvm::XVMInfinity);		   // v == infinity
		int mask   = _mm_movemask_ps(cmp) & 0x0000000F;			   // If x or y or z or w is infinity, the mask is non-zero
		return mask != 0;
	}

	XVM_INLINE bool XVM_CALLCONV isnan(Vec4 v)
	{
		__m128 dst	= _mm_cmpneq_ps(v.vec, v.vec);		 // v != v
		int	   mask = _mm_movemask_ps(dst) & 0x0000000F; // If x or y or z or w are NaN, the mask is non-zero
		return mask != 0;
	}

	XVM_INLINE Vec4 XVM_CALLCONV abs(Vec4 v)
	{
		// & the sign bit
		return _mm_and_ps(xvm::XVMMaskAbsoluteValue, v.vec);
	}

	XVM_INLINE Vec4 XVM_CALLCONV clamp(Vec4 v, Vec4 min, Vec4 max)
	{
		__m128 xmm0 = _mm_min_ps(v.vec, max.vec);
		__m128 xmm1 = _mm_max_ps(xmm0, min.vec);
		return xmm1;
	}

	XVM_INLINE Vec4 XVM_CALLCONV dot(Vec4 v1, Vec4 v2)
	{
		__m128 dot	 = _mm_mul_ps(v1.vec, v2.vec);
		__m128 shuff = XVM_SWIZZLE_PS(dot, _MM_SHUFFLE(2, 3, 0, 1)); // { y, x, w, z }
		dot			 = _mm_add_ps(dot, shuff);						 // { x+y, y+x, z+w, w+z }
		shuff		 = _mm_movehl_ps(shuff, dot);					 // { dot.fp2, dot.fp3, shuff.fp2, shuff.fp3 }
		dot			 = _mm_add_ss(dot, shuff);						 // { x+y+z+w, ?, ?, ? }
		return XVM_SWIZZLE_PS(dot, _MM_SHUFFLE(0, 0, 0, 0));		 // splat fp0
	}

	XVM_INLINE Vec4 XVM_CALLCONV length(Vec4 v)
	{
		Vec4 v2 = dot(v, v);
		return _mm_sqrt_ps(v2.vec);
	}

	XVM_INLINE Vec4 XVM_CALLCONV lerp(Vec4 v1, Vec4 v2, Vec4 s)
	{
		// x + s(y-x)
		__m128 sub = _mm_sub_ps(v2.vec, v1.vec);
		__m128 mul = _mm_mul_ps(sub, s.vec);
		return _mm_add_ps(v1.vec, mul);
	}

	XVM_INLINE Vec4 XVM_CALLCONV max(Vec4 v1, Vec4 v2)
	{
		return _mm_max_ps(v1.vec, v2.vec);
	}

	XVM_INLINE Vec4 XVM_CALLCONV min(Vec4 v1, Vec4 v2)
	{
		return _mm_min_ps(v1.vec, v2.vec);
	}

	XVM_INLINE Vec4 XVM_CALLCONV normalize(Vec4 v)
	{
		Vec4 l = length(v);
		return _mm_div_ps(v.vec, l.vec);
	}

	XVM_INLINE Vec4 XVM_CALLCONV sqrt(Vec4 v)
	{
		return _mm_sqrt_ps(v.vec);
	}
} // namespace xvm

template<typename T>
struct Vec4
{
	constexpr Vec4() noexcept
		: x(static_cast<T>(0))
		, y(static_cast<T>(0))
		, z(static_cast<T>(0))
		, w(static_cast<T>(0))
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

	constexpr T operator[](size_t i) const noexcept
	{
		assert(i < 4);
		return this->*Array[i];
	}

	constexpr T& operator[](size_t i) noexcept
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

	Vec4 operator-(const Vec4& v) const noexcept
	{
		return Vec4(x - v.x, y - v.y, z - v.z, w - v.w);
	}

	Vec4 operator*(const Vec4& v) const noexcept
	{
		return Vec4(x * v.x, y * v.y, z * v.z, w * v.w);
	}

	Vec4 operator/(T s) const noexcept
	{
		float inv = static_cast<T>(1) / s;
		return Vec4(x * inv, y * inv, z * inv, w * inv);
	}

	Vec4& operator+=(const Vec4& v) noexcept
	{
		x += v.x;
		y += v.y;
		z += v.z;
		w += v.w;
		return *this;
	}

	Vec4& operator-=(const Vec4& v) noexcept
	{
		x -= v.x;
		y -= v.y;
		z -= v.z;
		w -= v.w;
		return *this;
	}

	Vec4& operator*=(const Vec4& v) noexcept
	{
		x *= v.x;
		y *= v.y;
		z *= v.z;
		w *= v.w;
		return *this;
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

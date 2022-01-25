#pragma once
#include <cassert>
#include <cmath>
#include <algorithm>
#include "Intrinsics.h"

namespace xvm
{
	struct Vec2
	{
		Vec2() noexcept
			: vec(_mm_setzero_ps())
		{
		}
		Vec2(__m128 a) noexcept
			: vec(a)
		{
		}
		Vec2(float x, float y) noexcept
			: vec(_mm_set_ps(0.0f, 0.0f, y, x))
		{
		}
		Vec2(float v) noexcept
			: Vec2(v, v)
		{
		}

		float operator[](size_t Index) const noexcept
		{
			assert(Index < 2);
			return f32[Index];
		}

		float& operator[](size_t Index) noexcept
		{
			assert(Index < 2);
			return f32[Index];
		}

		union
		{
			__m128				  vec;
			float				  f32[4];
			xvm::ScalarSwizzle<0> x;
			xvm::ScalarSwizzle<1> y;
		};
	};

	Vec2 XVM_CALLCONV  operator-(Vec2 v);
	Vec2 XVM_CALLCONV  operator+(Vec2 v1, Vec2 v2);
	Vec2 XVM_CALLCONV  operator-(Vec2 v1, Vec2 v2);
	Vec2 XVM_CALLCONV  operator*(Vec2 v1, Vec2 v2);
	Vec2 XVM_CALLCONV  operator/(Vec2 v1, Vec2 v2);
	Vec2& XVM_CALLCONV operator+=(Vec2& v1, Vec2 v2);
	Vec2& XVM_CALLCONV operator-=(Vec2& v1, Vec2 v2);
	Vec2& XVM_CALLCONV operator*=(Vec2& v1, Vec2 v2);
	Vec2& XVM_CALLCONV operator/=(Vec2& v1, Vec2 v2);

	bool XVM_CALLCONV All(Vec2 v);
	bool XVM_CALLCONV Any(Vec2 v);
	bool XVM_CALLCONV IsFinite(Vec2 v);
	bool XVM_CALLCONV IsInfinite(Vec2 v);
	bool XVM_CALLCONV IsNan(Vec2 v);

	Vec2 XVM_CALLCONV Abs(Vec2 v);
	Vec2 XVM_CALLCONV Clamp(Vec2 v, Vec2 min, Vec2 max);
	Vec2 XVM_CALLCONV Dot(Vec2 v1, Vec2 v2);
	Vec2 XVM_CALLCONV Length(Vec2 v);
	Vec2 XVM_CALLCONV Lerp(Vec2 v1, Vec2 v2, Vec2 s);
	Vec2 XVM_CALLCONV Max(Vec2 v1, Vec2 v2);
	Vec2 XVM_CALLCONV Min(Vec2 v1, Vec2 v2);
	Vec2 XVM_CALLCONV Normalize(Vec2 v);
	Vec2 XVM_CALLCONV Sqrt(Vec2 v);

	XVM_INLINE Vec2 XVM_CALLCONV operator-(Vec2 v)
	{
		return _mm_xor_ps(v.vec, xvm::XVMMaskNegative);
	}

	XVM_INLINE Vec2 XVM_CALLCONV operator+(Vec2 v1, Vec2 v2)
	{
		return _mm_add_ps(v1.vec, v2.vec);
	}

	XVM_INLINE Vec2 XVM_CALLCONV operator-(Vec2 v1, Vec2 v2)
	{
		return _mm_sub_ps(v1.vec, v2.vec);
	}

	XVM_INLINE Vec2 XVM_CALLCONV operator*(Vec2 v1, Vec2 v2)
	{
		return _mm_mul_ps(v1.vec, v2.vec);
	}

	XVM_INLINE Vec2 XVM_CALLCONV operator/(Vec2 v1, Vec2 v2)
	{
		return _mm_div_ps(v1.vec, v2.vec);
	}

	XVM_INLINE Vec2& XVM_CALLCONV operator+=(Vec2& v1, Vec2 v2)
	{
		v1 = v1 + v2;
		return v1;
	}

	XVM_INLINE Vec2& XVM_CALLCONV operator-=(Vec2& v1, Vec2 v2)
	{
		v1 = v1 - v2;
		return v1;
	}

	XVM_INLINE Vec2& XVM_CALLCONV operator*=(Vec2& v1, Vec2 v2)
	{
		v1 = v1 * v2;
		return v1;
	}

	XVM_INLINE Vec2& XVM_CALLCONV operator/=(Vec2& v1, Vec2 v2)
	{
		v1 = v1 / v2;
		return v1;
	}

	XVM_INLINE bool XVM_CALLCONV All(Vec2 v)
	{
		__m128 cmp	= _mm_cmpneq_ps(v.vec, _mm_setzero_ps()); // v != 0
		int	   mask = _mm_movemask_ps(cmp) & 0x00000003;	  // If all is non-zero, the mask is exactly 0x00000003
		return mask == 0x00000003;
	}

	XVM_INLINE bool XVM_CALLCONV Any(Vec2 v)
	{
		__m128 cmp	= _mm_cmpneq_ps(v.vec, _mm_setzero_ps()); // v != 0
		int	   mask = _mm_movemask_ps(cmp) & 0x00000003;	  // If x or y are non-zero, the mask is non-zero
		return mask != 0;
	}

	XVM_INLINE bool XVM_CALLCONV IsFinite(Vec2 v)
	{
		__m128 cmp = _mm_and_ps(v.vec, xvm::XVMMaskAbsoluteValue); // Mask off the sign bit
		cmp		   = _mm_cmpneq_ps(cmp, xvm::XVMInfinity);		   // v != infinity
		int mask   = _mm_movemask_ps(cmp) & 0x00000003;			   // If x, y is finite, the mask is exactly 0x00000003
		return mask == 0x00000003;
	}

	XVM_INLINE bool XVM_CALLCONV IsInfinite(Vec2 v)
	{
		__m128 cmp = _mm_and_ps(v.vec, xvm::XVMMaskAbsoluteValue); // Mask off the sign bit then compare to infinity
		cmp		   = _mm_cmpeq_ps(cmp, xvm::XVMInfinity);		   // v == infinity
		int mask   = _mm_movemask_ps(cmp) & 0x00000003;			   // If x or y is infinity, the mask is non-zero
		return mask != 0;
	}

	XVM_INLINE bool XVM_CALLCONV IsNan(Vec2 v)
	{
		__m128 dst	= _mm_cmpneq_ps(v.vec, v.vec);		 // v != v
		int	   mask = _mm_movemask_ps(dst) & 0x00000003; // If x or y are nan, the mask is non-zero
		return mask != 0;
	}

	XVM_INLINE Vec2 XVM_CALLCONV Abs(Vec2 v)
	{
		// & the sign bit
		return _mm_and_ps(xvm::XVMMaskAbsoluteValue, v.vec);
	}

	XVM_INLINE Vec2 XVM_CALLCONV Clamp(Vec2 v, Vec2 min, Vec2 max)
	{
		__m128 xmm0 = _mm_min_ps(v.vec, max.vec);
		__m128 xmm1 = _mm_max_ps(xmm0, min.vec);
		return xmm1;
	}

	XVM_INLINE Vec2 XVM_CALLCONV Dot(Vec2 v1, Vec2 v2)
	{
		__m128 dot	 = _mm_mul_ps(v1.vec, v2.vec);
		__m128 shuff = XVM_SWIZZLE_PS(dot, _MM_SHUFFLE(1, 1, 1, 1)); // { y, y, y, y }
		dot			 = _mm_add_ss(dot, shuff);						 // { x+y, ?, ?, ? }
		return XVM_SWIZZLE_PS(dot, _MM_SHUFFLE(0, 0, 0, 0));		 // splat fp0
	}

	XVM_INLINE Vec2 XVM_CALLCONV Length(Vec2 v)
	{
		Vec2 v2 = Dot(v, v);
		return _mm_sqrt_ps(v2.vec);
	}

	XVM_INLINE Vec2 XVM_CALLCONV Lerp(Vec2 v1, Vec2 v2, Vec2 s)
	{
		// x + s(y-x)
		__m128 sub = _mm_sub_ps(v2.vec, v1.vec);
		__m128 mul = _mm_mul_ps(sub, s.vec);
		return _mm_add_ps(v1.vec, mul);
	}

	XVM_INLINE Vec2 XVM_CALLCONV Max(Vec2 v1, Vec2 v2)
	{
		return _mm_max_ps(v1.vec, v2.vec);
	}

	XVM_INLINE Vec2 XVM_CALLCONV Min(Vec2 v1, Vec2 v2)
	{
		return _mm_min_ps(v1.vec, v2.vec);
	}

	XVM_INLINE Vec2 XVM_CALLCONV Normalize(Vec2 v)
	{
		Vec2 l = Length(v);
		return _mm_div_ps(v.vec, l.vec);
	}

	XVM_INLINE Vec2 XVM_CALLCONV Sqrt(Vec2 v)
	{
		return _mm_sqrt_ps(v.vec);
	}
} // namespace xvm

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

	T operator[](size_t i) const noexcept
	{
		assert(i < 2);
		return this->*Array[i];
	}

	T& operator[](size_t i) noexcept
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

	Vec2 operator-(const Vec2& v) const noexcept
	{
		return Vec2(x - v.x, y - v.y);
	}

	Vec2 operator*(const Vec2& v) const noexcept
	{
		return Vec2(x * v.x, y * v.y);
	}

	Vec2 operator/(T s) const noexcept
	{
		float inv = static_cast<T>(1) / s;
		return Vec2(x * inv, y * inv);
	}

	Vec2& operator+=(const Vec2& v) noexcept
	{
		x += v.x;
		y += v.y;
		return *this;
	}

	Vec2& operator-=(const Vec2& v) noexcept
	{
		x -= v.x;
		y -= v.y;
		return *this;
	}

	Vec2& operator*=(const Vec2& v) noexcept
	{
		x *= v.x;
		y *= v.y;
		return *this;
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
[[nodiscard]] T Length(const Vec2<T>& v) noexcept
{
	return std::hypot(v.x, v.y);
}

template<typename T>
[[nodiscard]] Vec2<T> Abs(const Vec2<T>& v) noexcept
{
	return Vec2<T>(std::abs(v.x), std::abs(v.y));
}

template<typename T>
[[nodiscard]] T Dot(const Vec2<T>& v1, const Vec2<T>& v2) noexcept
{
	return v1.x * v2.x + v1.y * v2.y;
}

template<typename T>
[[nodiscard]] Vec2<T> Normalize(const Vec2<T>& v) noexcept
{
	return v / Length(v);
}

template<typename T>
[[nodiscard]] bool IsNan(const Vec2<T>& v) noexcept
{
	return std::isnan(v.x) || std::isnan(v.y);
}

template<typename T>
[[nodiscard]] Vec2<T> Clamp(const Vec2<T>& v, T min, T max) noexcept
{
	return Vec2(std::clamp(v.x, min, max), std::clamp(v.y, min, max));
}

using Vec2i = Vec2<int>;
using Vec2f = Vec2<float>;
using Vec2d = Vec2<double>;

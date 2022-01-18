#pragma once
#include <cassert>
#include <cmath>
#include <algorithm>
#include "Intrinsics.h"
#include "Vec3.h"
#include "Vec4.h"

namespace xvm
{
	// q = s + ia + jb + kc
	// Identities:
	// i^2 = j^2 = k^2 = ijk = -1
	// ij = k, jk = i, ki = j
	// ji = -k, kj = -i, ik = -j
	// ij = -ji = k
	// jk = -kj = i
	// ki = -ik = j
	// ixj = k, jxk = i, kxi = j cross product of 2 unit cartesian vectors

	// When a unit quaternion takes the form
	// q = [cos theta/2, sin theta/2 * v^hat]
	// and a pure quaternion storing a vector to be rotated takes the form
	// p = [0, p]
	// the pure quaternion
	// p' = q*p*q^-1
	// stores the rotated vector p'
	struct Quaternion
	{
		Quaternion() noexcept
			: vec(_mm_setzero_ps())
		{
		}
		Quaternion(__m128 a) noexcept
			: vec(a)
		{
		}
		Quaternion(float x, float y, float z, float w) noexcept
			: vec(_mm_set_ps(w, z, y, x))
		{
		}

		union
		{
			__m128 vec;
			// vector part
			xvm::ScalarSwizzle<0> x;
			xvm::ScalarSwizzle<1> y;
			xvm::ScalarSwizzle<2> z;
			// real part
			xvm::ScalarSwizzle<3> w;
		};
	};

	Quaternion XVM_CALLCONV	 operator+(Quaternion q1, Quaternion q2);
	Quaternion XVM_CALLCONV	 operator-(Quaternion q1, Quaternion q2);
	Quaternion XVM_CALLCONV	 operator*(Quaternion q1, Quaternion q2); // Quaternion product
	Quaternion XVM_CALLCONV	 operator*(Quaternion q, float v);
	Quaternion XVM_CALLCONV	 operator*(float v, Quaternion q);
	Quaternion& XVM_CALLCONV operator+=(Quaternion& q1, Quaternion q2);
	Quaternion& XVM_CALLCONV operator-=(Quaternion& q1, Quaternion q2);
	Quaternion& XVM_CALLCONV operator*=(Quaternion& q1, Quaternion q2);
	Quaternion& XVM_CALLCONV operator*=(Quaternion& q, float v);

	Quaternion XVM_CALLCONV conjugate(Quaternion q);
	Quaternion XVM_CALLCONV inverse(Quaternion q);
	Vec4 XVM_CALLCONV		dot(Quaternion q1, Quaternion q2);
	Vec4 XVM_CALLCONV		length(Quaternion q);
	Quaternion XVM_CALLCONV normalize(Quaternion q);
	Vec3 XVM_CALLCONV		mul(Vec3 v, Quaternion q);

	XVM_INLINE Quaternion XVM_CALLCONV operator+(Quaternion q1, Quaternion q2)
	{
		return _mm_add_ps(q1.vec, q2.vec);
	}

	XVM_INLINE Quaternion XVM_CALLCONV operator-(Quaternion q1, Quaternion q2)
	{
		return _mm_sub_ps(q1.vec, q2.vec);
	}

	XVM_INLINE Quaternion XVM_CALLCONV operator*(Quaternion q1, Quaternion q2)
	{
#if 0
	// Working the multiplication rule out of paper was "fun"...
	// https://1drv.ms/u/s!AoqfVczS5xmN-uZ1d1QcVOSpMfxb8g?e=6Oh6gX
	// Here's the link for the worked out multiplication, all you need to know is
	// the multiplication rule for ijk
	// [s_a, a]*[s_b, b] = [s_a * s_b - dot(a, b), s_a * b + s_b * a + a x b]
	// where s_ is the real part, a,b is vector part

	// Splat q1.w
	__m128 s_a = INTRINSICS_SWIZZLE_PS(q1.vec, _MM_SHUFFLE(3, 3, 3, 3));
	// Splat q2.w
	__m128 s_b = INTRINSICS_SWIZZLE_PS(q2.vec, _MM_SHUFFLE(3, 3, 3, 3));

	// dot(a, b), copy-pasta from dot(Vec3, Vec3) to keep consistency
	__m128 dot	 = _mm_mul_ps(q1.vec, q2.vec);
	__m128 shuff = INTRINSICS_SWIZZLE_PS(dot, _MM_SHUFFLE(2, 2, 2, 1)); // { y, z, z, z }
	dot			 = _mm_add_ss(dot, shuff);								// { x+y, ?, ?, ? }
	shuff		 = INTRINSICS_SWIZZLE_PS(dot, _MM_SHUFFLE(2, 2, 2, 2)); // { z, z, z, z }
	dot			 = _mm_add_ss(dot, shuff);								// { x+y+z, ?, ?, ? }
	__m128 aob	 = INTRINSICS_SWIZZLE_PS(dot, _MM_SHUFFLE(0, 0, 0, 0)); // splat fp0

	// s_a * s_b - dot(a, b)
	// real part
	__m128 real = _mm_sub_ps(_mm_mul_ps(s_a, s_b), aob);

	// a x b, copy-pasta from cross(Vec3, Vec3) to keep consistency
	__m128 v1_yzx = INTRINSICS_SWIZZLE_PS(q1.vec, _MM_SHUFFLE(3, 0, 2, 1));
	__m128 v2_yzx = INTRINSICS_SWIZZLE_PS(q2.vec, _MM_SHUFFLE(3, 0, 2, 1));
	__m128 cross  = _mm_sub_ps(_mm_mul_ps(q1.vec, v2_yzx), _mm_mul_ps(v1_yzx, q2.vec));
	__m128 axb	  = INTRINSICS_SWIZZLE_PS(cross, _MM_SHUFFLE(3, 0, 2, 1));

	// vector part
	__m128 s_ab = _mm_mul_ps(s_a, q2.vec);
	__m128 s_ba = _mm_mul_ps(s_b, q1.vec);
	__m128 vect = _mm_add_ps(s_ab, s_ba);
	vect		= _mm_add_ps(vect, axb);

	real = INTRINSICS_SHUFFLE_PS(real, vect, _MM_SHUFFLE(2, 2, 0, 0)); // { w, w, z, z }

	vect = INTRINSICS_SWIZZLE_PS(vect, _MM_SHUFFLE(2, 2, 1, 0)); // { x, y, z, z }

	return INTRINSICS_SHUFFLE_PS(vect, real, _MM_SHUFFLE(0, 2, 1, 0)); // { x, y, z, w }
#endif

		// Matrix implementation, refer to Eq 5.14, Section 5.17
		// [q1.w -q1.x -q1.y -q1.z] [q2.w]
		// [q1.x  q1.w -q1.z  q1.y] [q2.x]
		// [q1.y  q1.z  q1.w -q1.x] [q2.y]
		// [q1.z -q1.y  q1.x  q1.w] [q2.z]

		// Moved real part to last row and switched col0 and col3
		// [ q1.w -q1.z  q1.y q1.x] [q2.x]
		// [ q1.z  q1.w -q1.x q1.y] [q2.y]
		// [-q1.y  q1.x  q1.w q1.z] [q2.z]
		// [-q1.x -q1.y -q1.z q1.w] [q2.w]

		// Transpose matrices
		//							 [ q1.w  q1.z -q1.y -q1.x]
		// [q2.x, q2.y, q2.z q2.w] * [-q1.z  q1.w  q1.x -q1.y]
		//							 [ q1.y -q1.x  q1.w -q1.z]
		//							 [ q1.x  q1.y  q1.z  q1.w]

		constexpr xvm::XVM_UI32 R0SignMask = { { { 0x00000000, 0x00000000, 0x80000000, 0x80000000 } } };
		constexpr xvm::XVM_UI32 R1SignMask = { { { 0x80000000, 0x00000000, 0x00000000, 0x80000000 } } };
		constexpr xvm::XVM_UI32 R2SignMask = { { { 0x00000000, 0x80000000, 0x00000000, 0x80000000 } } };

		// Splat all q2 components
		__m128 x = XVM_SWIZZLE_PS(q2.vec, _MM_SHUFFLE(0, 0, 0, 0)); // [x, x, x, x]
		__m128 y = XVM_SWIZZLE_PS(q2.vec, _MM_SHUFFLE(1, 1, 1, 1)); // [y, y, y, y]
		__m128 z = XVM_SWIZZLE_PS(q2.vec, _MM_SHUFFLE(2, 2, 2, 2)); // [z, z, z, z]
		__m128 w = XVM_SWIZZLE_PS(q2.vec, _MM_SHUFFLE(3, 3, 3, 3)); // [w, w, w, w]

		__m128 r0 = XVM_SWIZZLE_PS(q1.vec, _MM_SHUFFLE(0, 1, 2, 3));
		r0		  = _mm_xor_ps(r0, R0SignMask);
		__m128 r1 = XVM_SWIZZLE_PS(q1.vec, _MM_SHUFFLE(1, 0, 3, 2));
		r1		  = _mm_xor_ps(r1, R1SignMask);
		__m128 r2 = XVM_SWIZZLE_PS(q1.vec, _MM_SHUFFLE(2, 3, 0, 1));
		r2		  = _mm_xor_ps(r2, R2SignMask);
		__m128 r3 = q1.vec;

		x = _mm_mul_ps(x, r0); // mul x by r0
		y = _mm_mul_ps(y, r1); // mul y by r1
		z = _mm_mul_ps(z, r2); // mul z by r2
		w = _mm_mul_ps(w, r3); // mul w by r3
		// sum
		x = _mm_add_ps(x, y);	 // x+y
		z = _mm_add_ps(z, w);	 // z+w;
		return _mm_add_ps(x, z); // x+y+z+w
	}

	XVM_INLINE Quaternion XVM_CALLCONV operator*(Quaternion q, float v)
	{
		return _mm_mul_ps(q.vec, _mm_set1_ps(v));
	}

	XVM_INLINE Quaternion XVM_CALLCONV operator*(float v, Quaternion q)
	{
		return q * v;
	}

	XVM_INLINE Quaternion& XVM_CALLCONV operator+=(Quaternion& q1, Quaternion q2)
	{
		q1 = q1 + q2;
		return q1;
	}

	XVM_INLINE Quaternion& XVM_CALLCONV operator-=(Quaternion& q1, Quaternion q2)
	{
		q1 = q1 - q2;
		return q1;
	}

	XVM_INLINE Quaternion& XVM_CALLCONV operator*=(Quaternion& q1, Quaternion q2)
	{
		q1 = q1 * q2;
		return q1;
	}

	XVM_INLINE Quaternion& XVM_CALLCONV operator*=(Quaternion& q, float v)
	{
		q = q * v;
		return q;
	}

	XVM_INLINE Quaternion XVM_CALLCONV conjugate(Quaternion q)
	{
		return _mm_xor_ps(q.vec, xvm::XVMMaskXYZNegative);
	}

	XVM_INLINE Quaternion XVM_CALLCONV inverse(Quaternion q)
	{
		// q*qq^-1 = q*
		// |q|^2q^-1 = q*
		// q^-1 = q* / |q|^2
		Quaternion qstar	= conjugate(q);
		Vec4	   lengthSq = length(q);
		lengthSq *= lengthSq;

		return _mm_div_ps(qstar.vec, lengthSq.vec);
	}

	XVM_INLINE Vec4 XVM_CALLCONV dot(Quaternion q1, Quaternion q2)
	{
		__m128 dot	 = _mm_mul_ps(q1.vec, q2.vec);
		__m128 shuff = XVM_SWIZZLE_PS(dot, _MM_SHUFFLE(2, 3, 0, 1)); // y, x, w, z
		dot			 = _mm_add_ps(dot, shuff);						 // { x+y, y+x, z+w, w+z }
		shuff		 = _mm_movehl_ps(shuff, dot);					 // { dot.fp2, dot.fp3, shuff.fp2, shuff.fp3 }
		dot			 = _mm_add_ss(dot, shuff);						 // { x+y+z+w, ?, ?, ? }
		return XVM_SWIZZLE_PS(dot, _MM_SHUFFLE(0, 0, 0, 0));		 // splat fp0
	}

	XVM_INLINE Vec4 XVM_CALLCONV length(Quaternion q)
	{
		Vec4 v2 = dot(q, q);
		return _mm_sqrt_ps(v2.vec);
	}

	XVM_INLINE Quaternion XVM_CALLCONV normalize(Quaternion q)
	{
		Vec4 l = length(q);
		return _mm_div_ps(q.vec, l.vec);
	}

	XVM_INLINE Vec3 XVM_CALLCONV mul(Vec3 v, Quaternion q)
	{
		// https://blog.molecular-matters.com/2013/05/24/a-faster-quaternion-vector-multiplication/

		__m128 t	  = xvm::_xvm_mm_cross_ps(q.vec, v.vec);
		t			  = _mm_add_ps(t, t);
		__m128 mul	  = _mm_mul_ps(XVM_SWIZZLE_PS(q.vec, _MM_SHUFFLE(3, 3, 3, 3)), t);
		__m128 vprime = _mm_add_ps(v.vec, mul);
		t			  = xvm::_xvm_mm_cross_ps(q.vec, t);
		vprime		  = _mm_add_ps(vprime, t);
		return vprime;
	}
} // namespace xvm

struct Quaternion
{
	constexpr Quaternion() noexcept
		: x(0.0f)
		, y(0.0f)
		, z(0.0f)
		, w(1.0f)
	{
	}
	constexpr Quaternion(float x, float y, float z, float w) noexcept
		: x(x)
		, y(y)
		, z(z)
		, w(w)
	{
	}

	float x;
	float y;
	float z;
	float w;
};

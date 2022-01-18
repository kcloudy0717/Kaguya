#pragma once
#include <cassert>
#include <cmath>
#include <algorithm>
#include "Intrinsics.h"
#include "Vec4.h"

namespace xvm
{
	struct Matrix4x4
	{
		Matrix4x4()
		{
			r[0] = xvm::XVMIdentityR0;
			r[1] = xvm::XVMIdentityR1;
			r[2] = xvm::XVMIdentityR2;
			r[3] = xvm::XVMIdentityR3;
		}
		Matrix4x4(
			__m128 r0,
			__m128 r1,
			__m128 r2,
			__m128 r3)
		{
			r[0] = r0;
			r[1] = r1;
			r[2] = r2;
			r[3] = r3;
		}
		Matrix4x4(
			float m00,
			float m01,
			float m02,
			float m03,
			float m10,
			float m11,
			float m12,
			float m13,
			float m20,
			float m21,
			float m22,
			float m23,
			float m30,
			float m31,
			float m32,
			float m33)
		{
			r[0] = _mm_set_ps(m03, m02, m01, m00);
			r[1] = _mm_set_ps(m13, m12, m11, m10);
			r[2] = _mm_set_ps(m23, m22, m21, m20);
			r[3] = _mm_set_ps(m33, m32, m31, m30);
		}
		Matrix4x4(float floats[16])
			: Matrix4x4(
				  floats[0],
				  floats[1],
				  floats[2],
				  floats[3],
				  floats[4],
				  floats[5],
				  floats[6],
				  floats[7],
				  floats[8],
				  floats[9],
				  floats[10],
				  floats[11],
				  floats[12],
				  floats[13],
				  floats[14],
				  floats[15])
		{
		}

		union
		{
			__m128 r[4];
			struct
			{
				union
				{
					__m128				  vec;
					xvm::ScalarSwizzle<0> x;
					xvm::ScalarSwizzle<1> y;
					xvm::ScalarSwizzle<2> z;
					xvm::ScalarSwizzle<3> w;
				} r0;
				union
				{
					__m128				  vec;
					xvm::ScalarSwizzle<0> x;
					xvm::ScalarSwizzle<1> y;
					xvm::ScalarSwizzle<2> z;
					xvm::ScalarSwizzle<3> w;
				} r1;
				union
				{
					__m128				  vec;
					xvm::ScalarSwizzle<0> x;
					xvm::ScalarSwizzle<1> y;
					xvm::ScalarSwizzle<2> z;
					xvm::ScalarSwizzle<3> w;
				} r2;
				union
				{
					__m128				  vec;
					xvm::ScalarSwizzle<0> x;
					xvm::ScalarSwizzle<1> y;
					xvm::ScalarSwizzle<2> z;
					xvm::ScalarSwizzle<3> w;
				} r3;
			};
		};
	};

	Matrix4x4 XVM_CALLCONV	operator-(Matrix4x4 m);
	Matrix4x4 XVM_CALLCONV	operator+(Matrix4x4 m1, Matrix4x4 m2);
	Matrix4x4 XVM_CALLCONV	operator-(Matrix4x4 m1, Matrix4x4 m2);
	Matrix4x4 XVM_CALLCONV	operator*(Matrix4x4 m, float v);
	Matrix4x4 XVM_CALLCONV	operator*(float v, Matrix4x4 m);
	Matrix4x4 XVM_CALLCONV	operator*(Matrix4x4 m1, Matrix4x4 m2);
	Matrix4x4 XVM_CALLCONV	operator/(Matrix4x4 m, float v);
	Matrix4x4& XVM_CALLCONV operator+=(Matrix4x4& m1, Matrix4x4 m2);
	Matrix4x4& XVM_CALLCONV operator-=(Matrix4x4& m1, Matrix4x4 m2);
	Matrix4x4& XVM_CALLCONV operator*=(Matrix4x4& m, float v);
	Matrix4x4& XVM_CALLCONV operator*=(Matrix4x4& m1, Matrix4x4 m2);
	Matrix4x4& XVM_CALLCONV operator/=(Matrix4x4& m, float v);

	// Treats v as a row vector matrix [1x4] * [4x4]
	// returns a [1x4] row vector
	Vec4 XVM_CALLCONV	   Mul(Vec4 v, Matrix4x4 m);
	Matrix4x4 XVM_CALLCONV Mul(Matrix4x4 m1, Matrix4x4 m2);

	Matrix4x4 XVM_CALLCONV Transpose(Matrix4x4 m);

	// Transformations
	Matrix4x4 XVM_CALLCONV Translation(float x, float y, float z);

	Matrix4x4 XVM_CALLCONV Scale(float x, float y, float z);

	Matrix4x4 XVM_CALLCONV RotationX(float angle);
	Matrix4x4 XVM_CALLCONV RotationY(float angle);
	Matrix4x4 XVM_CALLCONV RotationZ(float angle);

	XVM_INLINE Matrix4x4 XVM_CALLCONV operator-(Matrix4x4 m)
	{
		__m128 nx = _mm_xor_ps(m.r[0], xvm::XVMMaskNegative);
		__m128 ny = _mm_xor_ps(m.r[1], xvm::XVMMaskNegative);
		__m128 nz = _mm_xor_ps(m.r[2], xvm::XVMMaskNegative);
		__m128 nw = _mm_xor_ps(m.r[3], xvm::XVMMaskNegative);
		return { nx, ny, nz, nw };
	}

	XVM_INLINE Matrix4x4 XVM_CALLCONV operator+(Matrix4x4 m1, Matrix4x4 m2)
	{
		__m128 r0 = _mm_add_ps(m1.r[0], m2.r[0]);
		__m128 r1 = _mm_add_ps(m1.r[1], m2.r[1]);
		__m128 r2 = _mm_add_ps(m1.r[2], m2.r[2]);
		__m128 r3 = _mm_add_ps(m1.r[3], m2.r[3]);
		return { r0, r1, r2, r3 };
	}

	XVM_INLINE Matrix4x4 XVM_CALLCONV operator-(Matrix4x4 m1, Matrix4x4 m2)
	{
		__m128 r0 = _mm_sub_ps(m1.r[0], m2.r[0]);
		__m128 r1 = _mm_sub_ps(m1.r[1], m2.r[1]);
		__m128 r2 = _mm_sub_ps(m1.r[2], m2.r[2]);
		__m128 r3 = _mm_sub_ps(m1.r[3], m2.r[3]);
		return { r0, r1, r2, r3 };
	}

	XVM_INLINE Matrix4x4 XVM_CALLCONV operator*(Matrix4x4 m, float v)
	{
		__m128 s  = _mm_set1_ps(v);
		__m128 r0 = _mm_mul_ps(m.r[0], s);
		__m128 r1 = _mm_mul_ps(m.r[1], s);
		__m128 r2 = _mm_mul_ps(m.r[2], s);
		__m128 r3 = _mm_mul_ps(m.r[3], s);
		return { r0, r1, r2, r3 };
	}

	XVM_INLINE Matrix4x4 XVM_CALLCONV operator*(float v, Matrix4x4 m)
	{
		// scalar multiplication is commutative
		return m * v;
	}

	XVM_INLINE Matrix4x4 XVM_CALLCONV operator*(Matrix4x4 m1, Matrix4x4 m2)
	{
		return Mul(m1, m2);
	}

	XVM_INLINE Matrix4x4 XVM_CALLCONV operator/(Matrix4x4 m, float v)
	{
		__m128 s  = _mm_set1_ps(v);
		__m128 r0 = _mm_div_ps(m.r[0], s);
		__m128 r1 = _mm_div_ps(m.r[1], s);
		__m128 r2 = _mm_div_ps(m.r[2], s);
		__m128 r3 = _mm_div_ps(m.r[3], s);
		return { r0, r1, r2, r3 };
	}

	XVM_INLINE Matrix4x4& XVM_CALLCONV operator+=(Matrix4x4& m1, Matrix4x4 m2)
	{
		m1 = m1 + m2;
		return m1;
	}

	XVM_INLINE Matrix4x4& XVM_CALLCONV operator-=(Matrix4x4& m1, Matrix4x4 m2)
	{
		m1 = m1 - m2;
		return m1;
	}

	XVM_INLINE Matrix4x4& XVM_CALLCONV operator*=(Matrix4x4& m, float v)
	{
		m = m * v;
		return m;
	}

	XVM_INLINE Matrix4x4& XVM_CALLCONV operator*=(Matrix4x4& m1, Matrix4x4 m2)
	{
		m1 = m1 * m2;
		return m1;
	}

	XVM_INLINE Matrix4x4& XVM_CALLCONV operator/=(Matrix4x4& m, float v)
	{
		m = m / v;
		return m;
	}

	XVM_INLINE Vec4 XVM_CALLCONV Mul(Vec4 v, Matrix4x4 m)
	{
		//				[a b c d]
		// [x y z w] *	[e f g h]
		//				[i j k l]
		//				[m n o p]
		//
		// x' = x * a + y * e + z * i + w * m
		// y' = x * b + y * f + z * j + w * n
		// z' = x * c + y * g + z * k + w * o
		// w' = x * d + y * h + z * l + w * p
		// Splat all vector components
		__m128 x = XVM_SWIZZLE_PS(v.vec, _MM_SHUFFLE(0, 0, 0, 0)); // [x, x, x, x]
		__m128 y = XVM_SWIZZLE_PS(v.vec, _MM_SHUFFLE(1, 1, 1, 1)); // [y, y, y, y]
		__m128 z = XVM_SWIZZLE_PS(v.vec, _MM_SHUFFLE(2, 2, 2, 2)); // [z, z, z, z]
		__m128 w = XVM_SWIZZLE_PS(v.vec, _MM_SHUFFLE(3, 3, 3, 3)); // [w, w, w, w]

		x = _mm_mul_ps(x, m.r[0]); // mul x by r0
		y = _mm_mul_ps(y, m.r[1]); // mul y by r1
		z = _mm_mul_ps(z, m.r[2]); // mul z by r2
		w = _mm_mul_ps(w, m.r[3]); // mul w by r3
		// sum
		x = _mm_add_ps(x, y);	 // x+y
		z = _mm_add_ps(z, w);	 // z+w;
		return _mm_add_ps(x, z); // x+y+z+w
	}

	XVM_INLINE Matrix4x4 XVM_CALLCONV Mul(Matrix4x4 m1, Matrix4x4 m2)
	{
		__m128 r[4];

		// See mul(Vec4, Matrix4x4)
		for (int i = 0; i < 4; ++i)
		{
			__m128 x = XVM_SWIZZLE_PS(m1.r[i], _MM_SHUFFLE(0, 0, 0, 0)); // [x, x, x, x]
			__m128 y = XVM_SWIZZLE_PS(m1.r[i], _MM_SHUFFLE(1, 1, 1, 1)); // [y, y, y, y]
			__m128 z = XVM_SWIZZLE_PS(m1.r[i], _MM_SHUFFLE(2, 2, 2, 2)); // [z, z, z, z]
			__m128 w = XVM_SWIZZLE_PS(m1.r[i], _MM_SHUFFLE(3, 3, 3, 3)); // [w, w, w, w]

			x = _mm_mul_ps(x, m2.r[0]); // mul x by r0
			y = _mm_mul_ps(y, m2.r[1]); // mul y by r1
			z = _mm_mul_ps(z, m2.r[2]); // mul z by r2
			w = _mm_mul_ps(w, m2.r[3]); // mul w by r3
			// sum
			x	 = _mm_add_ps(x, y); // x+y
			z	 = _mm_add_ps(z, w); // z+w;
			r[i] = _mm_add_ps(x, z); // x+y+z+w
		}

		return Matrix4x4(r[0], r[1], r[2], r[3]);
	}

	XVM_INLINE Matrix4x4 XVM_CALLCONV Transpose(Matrix4x4 m)
	{
		// I worked this out on paper, is pretty interesting on how they arrived at the solution
		_MM_TRANSPOSE4_PS(m.r[0], m.r[1], m.r[2], m.r[3]);
		return m;
	}

	XVM_INLINE Matrix4x4 XVM_CALLCONV Translation(float x, float y, float z)
	{
		// row-major | col-major
		// [1 0 0 0] | [1 0 0 x]
		// [0 1 0 0] | [0 1 0 y]
		// [0 0 1 0] | [0 0 1 z]
		// [x y z 1] | [0 0 0 1]
		__m128 r[4];
		r[0] = xvm::XVMIdentityR0;
		r[1] = xvm::XVMIdentityR1;
		r[2] = xvm::XVMIdentityR2;
		r[3] = _mm_set_ps(1.0f, z, y, x);
		return Matrix4x4(r[0], r[1], r[2], r[3]);
	}

	XVM_INLINE Matrix4x4 XVM_CALLCONV Scale(float x, float y, float z)
	{
		// row-major, col-major transpose is same
		// [x 0 0 0]
		// [0 y 0 0]
		// [0 0 z 0]
		// [0 0 0 1]
		__m128 r[4];
		r[0] = _mm_set_ps(0, 0, 0, x);
		r[1] = _mm_set_ps(0, 0, y, 0);
		r[2] = _mm_set_ps(0, z, 0, 0);
		r[3] = xvm::XVMIdentityR3;
		return Matrix4x4(r[0], r[1], r[2], r[3]);
	}

	XVM_INLINE Matrix4x4 XVM_CALLCONV RotationX(float angle)
	{
		float sinTheta = std::sin(angle);
		float cosTheta = std::cos(angle);

		return Matrix4x4(1, 0, 0, 0, 0, cosTheta, sinTheta, 0, 0, -sinTheta, cosTheta, 0, 0, 0, 0, 1);
	}
	XVM_INLINE Matrix4x4 XVM_CALLCONV RotationY(float angle)
	{
		float sinTheta = std::sin(angle);
		float cosTheta = std::cos(angle);

		return Matrix4x4(cosTheta, 0, -sinTheta, 0, 0, 1, 0, 0, sinTheta, 0, cosTheta, 0, 0, 0, 0, 1);
	}
	XVM_INLINE Matrix4x4 XVM_CALLCONV RotationZ(float angle)
	{
		float sinTheta = std::sin(angle);
		float cosTheta = std::cos(angle);

		return Matrix4x4(cosTheta, sinTheta, 0, 0, -sinTheta, cosTheta, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
	}
} // namespace xvm

struct Matrix4x4
{
	// clang-format off
	Matrix4x4() noexcept
		: _11(1.0f) , _12(0.0f) , _13(0.0f) , _14(0.0f)
		, _21(0.0f) , _22(1.0f) , _23(0.0f) , _24(0.0f)
		, _31(0.0f) , _32(0.0f) , _33(1.0f) , _34(0.0f)
		, _41(0.0f) , _42(0.0f) , _43(0.0f) , _44(1.0f)
	{
	}
	Matrix4x4(const Vec4f& Row0, const Vec4f& Row1, const Vec4f& Row2, const Vec4f Row3) noexcept
		: _11(Row0.x) , _12(Row0.y) , _13(Row0.z) , _14(Row0.w)
		, _21(Row1.x) , _22(Row1.y) , _23(Row1.z) , _24(Row1.w)
		, _31(Row2.x) , _32(Row2.y) , _33(Row2.z) , _34(Row2.w)
		, _41(Row3.x) , _42(Row3.y) , _43(Row3.z) , _44(Row3.w)
	{
	}
	// clang-format on

	[[nodiscard]] Vec3f Right() const noexcept
	{
		return Vec3f(_11, _12, _13);
	}

	[[nodiscard]] Vec3f Left() const noexcept
	{
		return -Vec3f(_11, _12, _13);
	}

	[[nodiscard]] Vec3f Up() const noexcept
	{
		return Vec3f(_21, _22, _23);
	}

	[[nodiscard]] Vec3f Down() const noexcept
	{
		return -Vec3f(_21, _22, _23);
	}

	[[nodiscard]] Vec3f Forward() const noexcept
	{
		return Vec3f(_31, _32, _33);
	}
	[[nodiscard]] Vec3f Back() const noexcept
	{
		return -Vec3f(_31, _32, _33);
	}

	float _11, _12, _13, _14;
	float _21, _22, _23, _24;
	float _31, _32, _33, _34;
	float _41, _42, _43, _44;
};

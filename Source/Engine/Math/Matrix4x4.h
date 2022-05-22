#pragma once
#include <cmath>
#include "Vec2.h"
#include "Vec3.h"
#include "Vec4.h"

namespace Math
{
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
		Matrix4x4(
			float _11, float _12, float _13, float _14,
			float _21, float _22, float _23, float _24,
			float _31, float _32, float _33, float _34,
			float _41, float _42, float _43, float _44) noexcept
			: _11(_11) , _12(_12) , _13(_13) , _14(_14)
			, _21(_21) , _22(_22) , _23(_23) , _24(_24)
			, _31(_31) , _32(_32) , _33(_33) , _34(_34)
			, _41(_41) , _42(_42) , _43(_43) , _44(_44)
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

		[[nodiscard]] static Matrix4x4 Translation(float x, float y, float z) noexcept
		{
			return Matrix4x4(
				Vec4f(1.0f, 0.0f, 0.0f, 0.0f),
				Vec4f(0.0f, 1.0f, 0.0f, 0.0f),
				Vec4f(0.0f, 0.0f, 1.0f, 0.0f),
				Vec4f(x, y, z, 1.0f));
		}

		[[nodiscard]] static Matrix4x4 Scale(float x, float y, float z) noexcept
		{
			return Matrix4x4(
				Vec4f(x, 0.0f, 0.0f, 0.0f),
				Vec4f(0.0f, y, 0.0f, 0.0f),
				Vec4f(0.0f, 0.0f, z, 0.0f),
				Vec4f(0.0f, 0.0f, 0.0f, 1.0f));
		}

		[[nodiscard]] static Matrix4x4 RotateX(float Angle) noexcept
		{
			const float Sin = std::sin(Angle);
			const float Cos = std::cos(Angle);
			return Matrix4x4(
				Vec4f(1.0f, 0.0f, 0.0f, 0.0f),
				Vec4f(0.0f, Cos, Sin, 0.0f),
				Vec4f(0.0f, -Sin, Cos, 0.0f),
				Vec4f(0.0f, 0.0f, 0.0f, 1.0f));
		}

		[[nodiscard]] static Matrix4x4 RotateY(float Angle) noexcept
		{
			const float Sin = std::sin(Angle);
			const float Cos = std::cos(Angle);
			return Matrix4x4(
				Vec4f(Cos, 0.0f, -Sin, 0.0f),
				Vec4f(0.0f, 1.0f, 0.0f, 0.0f),
				Vec4f(Sin, 0.0f, Cos, 0.0f),
				Vec4f(0.0f, 0.0f, 0.0f, 1.0f));
		}

		[[nodiscard]] static Matrix4x4 RotateZ(float Angle) noexcept
		{
			const float Sin = std::sin(Angle);
			const float Cos = std::cos(Angle);
			return Matrix4x4(
				Vec4f(Cos, Sin, 0.0f, 0.0f),
				Vec4f(-Sin, Cos, 0.0f, 0.0f),
				Vec4f(0.0f, 0.0f, 1.0f, 0.0f),
				Vec4f(0.0f, 0.0f, 0.0f, 1.0f));
		}

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

		[[nodiscard]] Vec4f GetRow(size_t Index) const noexcept
		{
			switch (Index)
			{
			case 0:
				return Vec4f(_11, _12, _13, _14);
			case 1:
				return Vec4f(_21, _22, _23, _24);
			case 2:
				return Vec4f(_31, _32, _33, _34);
			case 3:
				return Vec4f(_41, _42, _43, _44);
			default:
				return Vec4f(0.0f, 0.0f, 0.0f, 0.0f);
			}
		}

		float _11, _12, _13, _14;
		float _21, _22, _23, _24;
		float _31, _32, _33, _34;
		float _41, _42, _43, _44;
	};

	// Intrinsics
	Vec4f	  mul(const Vec4f& v, const Matrix4x4& m) noexcept;
	Matrix4x4 mul(const Matrix4x4& a, const Matrix4x4& b) noexcept;

} // namespace Math

#include "Matrix4x4.inl"

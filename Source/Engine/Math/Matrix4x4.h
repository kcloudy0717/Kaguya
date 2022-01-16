#pragma once
#include "Vec4.h"

struct Matrix4x4
{
	// clang-format off
	constexpr Matrix4x4() noexcept
		: _11(0.0f) , _12(0.0f) , _13(0.0f) , _14(0.0f)
		, _21(0.0f) , _22(0.0f) , _23(0.0f) , _24(0.0f)
		, _31(0.0f) , _32(0.0f) , _33(0.0f) , _34(0.0f)
		, _41(0.0f) , _42(0.0f) , _43(0.0f) , _44(0.0f)
	{
	}
	// clang-format on

	float  operator()(size_t Row, size_t Column) const noexcept { return m[Row][Column]; }
	float& operator()(size_t Row, size_t Column) noexcept { return m[Row][Column]; }

	union
	{
		struct
		{
			float _11, _12, _13, _14;
			float _21, _22, _23, _24;
			float _31, _32, _33, _34;
			float _41, _42, _43, _44;
		};
		float m[4][4];
	};

private:
	static constexpr float Matrix4x4::*Row0[]{ &Matrix4x4::_11, &Matrix4x4::_12, &Matrix4x4::_13, &Matrix4x4::_14 };
	static constexpr float Matrix4x4::*Row1[]{ &Matrix4x4::_21, &Matrix4x4::_22, &Matrix4x4::_23, &Matrix4x4::_24 };
	static constexpr float Matrix4x4::*Row2[]{ &Matrix4x4::_31, &Matrix4x4::_32, &Matrix4x4::_33, &Matrix4x4::_34 };
	static constexpr float Matrix4x4::*Row3[]{ &Matrix4x4::_41, &Matrix4x4::_42, &Matrix4x4::_43, &Matrix4x4::_44 };
};

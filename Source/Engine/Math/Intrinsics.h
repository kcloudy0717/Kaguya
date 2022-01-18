#pragma once
#include <cmath>
#include <cstdint>
#include <immintrin.h>
#include <intrin.h>
#include <xmmintrin.h>

#define XVM_INLINE				  inline

#define XVM_CALLCONV			  __vectorcall

// _MM_SHUFFLE(3, 2, 1, 0) -> [x, y, z, w]
#define XVM_SWIZZLE_PS(v, c)	  _mm_shuffle_ps(v, v, c)

// { v1[fp0], v1[fp1], v2[fp2], v2[fp3] }
#define XVM_SHUFFLE_PS(v1, v2, c) _mm_shuffle_ps(v1, v2, c)

// https://stackoverflow.com/questions/51641131/how-to-achieve-vector-swizzling-in-c
// https://docs.microsoft.com/en-us/cpp/cpp/m128?view=msvc-160

namespace xvm
{
	// TODO: Add swizzling to Vec2,Vec3,Vec4
	template<int I>
	struct ScalarSwizzle
	{
		operator float() const noexcept { return f32[I]; }

		ScalarSwizzle& operator=(float v)
		{
			f32[I] = v;
			return *this;
		}

	private:
		union
		{
			__m128 vec;
			float  f32[4];
		};
	};
} // namespace xvm

namespace xvm
{
#ifndef XVM_GLOBALCONST
#define XVM_GLOBALCONST extern const __declspec(selectany)
#endif

	union FP32
	{
		uint32_t u;
		float	 f;
		struct
		{
			uint32_t Mantissa : 23;
			uint32_t Exponent : 8;
			uint32_t Sign	  : 1;
		};
	};

	__declspec(align(16)) struct XVM_FP32
	{
		union
		{
			float  fp32[4];
			__m128 v;
		};

		operator __m128() const noexcept { return v; }
		operator const float*() const noexcept { return fp32; }
		operator __m128i() const noexcept { return _mm_castps_si128(v); }
		operator __m128d() const noexcept { return _mm_castps_pd(v); }
	};

	__declspec(align(16)) struct XVM_I32
	{
		union
		{
			int32_t i32[4];
			__m128	v;
		};

		operator __m128() const noexcept { return v; }
		operator __m128i() const noexcept { return _mm_castps_si128(v); }
		operator __m128d() const noexcept { return _mm_castps_pd(v); }
	};

	__declspec(align(16)) struct XVM_UI32
	{
		union
		{
			uint32_t ui32[4];
			__m128	 v;
		};

		operator __m128() const noexcept { return v; }
		operator __m128i() const noexcept { return _mm_castps_si128(v); }
		operator __m128d() const noexcept { return _mm_castps_pd(v); }
	};

	XVM_GLOBALCONST FP32 XVMFP32Infinity		 = { 0x7F800000 };
	XVM_GLOBALCONST FP32 XVMFP32NegativeInfinity = { 0xFF800000 };

	XVM_GLOBALCONST XVM_FP32 XVMIdentityR0 = { { { 1.0f, 0.0f, 0.0f, 0.0f } } };
	XVM_GLOBALCONST XVM_FP32 XVMIdentityR1 = { { { 0.0f, 1.0f, 0.0f, 0.0f } } };
	XVM_GLOBALCONST XVM_FP32 XVMIdentityR2 = { { { 0.0f, 0.0f, 1.0f, 0.0f } } };
	XVM_GLOBALCONST XVM_FP32 XVMIdentityR3 = { { { 0.0f, 0.0f, 0.0f, 1.0f } } };

	XVM_GLOBALCONST XVM_I32	 XVMMaskAbsoluteValue = { { { 0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff } } };
	XVM_GLOBALCONST XVM_UI32 XVMMaskNegative	  = { { { 0x80000000, 0x80000000, 0x80000000, 0x80000000 } } };
	XVM_GLOBALCONST XVM_UI32 XVMMaskXYZNegative	  = { { { 0x80000000, 0x80000000, 0x80000000, 0x00000000 } } };
	XVM_GLOBALCONST XVM_UI32 XVMInfinity		  = { { { 0x7F800000, 0x7F800000, 0x7F800000, 0x7F800000 } } };
	XVM_GLOBALCONST XVM_UI32 XVMNegativeInfinity  = { { { 0xFF800000, 0xFF800000, 0xFF800000, 0xFF800000 } } };
	XVM_GLOBALCONST XVM_I32	 XVMNaNS			  = { { { 0x7F800001, 0x7F800001, 0x7F800001, 0x7F800001 } } };
	XVM_GLOBALCONST XVM_I32	 XVMNaNQ			  = { { { 0x7FC00000, 0x7FC00000, 0x7FC00000, 0x7FC00000 } } };

	XVM_GLOBALCONST XVM_FP32 XVMQuaternionUnitI = { { { 1.0f, 0.0f, 0.0f, 0.0f } } };
	XVM_GLOBALCONST XVM_FP32 XVMQuaternionUnitJ = { { { 0.0f, 1.0f, 0.0f, 0.0f } } };
	XVM_GLOBALCONST XVM_FP32 XVMQuaternionUnitK = { { { 0.0f, 0.0f, 1.0f, 0.0f } } };

#undef XVM_GLOBALCONST
} // namespace xvm

#include "Intrinsics.inl"
//
//#undef XVM_SWIZZLE_PS
//#undef XVM_CALLCONV
//#undef XVM_INLINE

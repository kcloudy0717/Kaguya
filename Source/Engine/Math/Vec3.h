#pragma once
#include "Vec.h"

namespace Math
{
	template<typename T>
	struct [[nodiscard]] Vec<T, 3>
	{
		constexpr Vec() noexcept
			: x(static_cast<T>(0))
			, y(static_cast<T>(0))
			, z(static_cast<T>(0))
		{
		}
		constexpr Vec(T x, T y, T z) noexcept
			: x(x)
			, y(y)
			, z(z)
		{
		}
		constexpr Vec(T v) noexcept
			: Vec(v, v, v)
		{
		}
		constexpr Vec(const Vec<T, 2>& xy, T z) noexcept
			: x(xy.x)
			, y(xy.y)
			, z(z)
		{
		}
		constexpr Vec(const Vec<T, 4>& v) noexcept
			: x(v.x)
			, y(v.y)
			, z(v.z)
		{
		}
		template<typename U>
		constexpr Vec(const Vec<U, 3>& v) noexcept
			: x(static_cast<T>(v.x))
			, y(static_cast<T>(v.y))
			, z(static_cast<T>(v.z))
		{
		}

		T*		 data() noexcept { return reinterpret_cast<T*>(this); }
		const T* data() const noexcept { return reinterpret_cast<const T*>(this); }

		constexpr T operator[](size_t i) const noexcept
		{
			return this->*Array[i];
		}

		constexpr T& operator[](size_t i) noexcept
		{
			return this->*Array[i];
		}

		T x, y, z;

	private:
		static constexpr T Vec::*Array[]{ &Vec::x, &Vec::y, &Vec::z };
	};

#define DEFINE_UNARY_OPERATOR(op)                                \
	template<typename T>                                         \
	constexpr Vec<T, 3> operator op(const Vec<T, 3>& a) noexcept \
	{                                                            \
		return Vec<T, 3>(op a.x, op a.y, op a.z);                \
	}

#define DEFINE_BINARY_OPERATORS(op)                                                  \
	/* Vector-Vector op */                                                           \
	template<typename T>                                                             \
	constexpr Vec<T, 3> operator op(const Vec<T, 3>& a, const Vec<T, 3>& b) noexcept \
	{                                                                                \
		return Vec<T, 3>(a.x op b.x, a.y op b.y, a.z op b.z);                        \
	}                                                                                \
	/* Vector-Scalar op */                                                           \
	template<typename T>                                                             \
	constexpr Vec<T, 3> operator op(const Vec<T, 3>& a, T b) noexcept                \
	{                                                                                \
		return Vec<T, 3>(a.x op b, a.y op b, a.z op b);                              \
	}                                                                                \
	/* Scalar-Vector op */                                                           \
	template<typename T>                                                             \
	constexpr Vec<T, 3> operator op(T a, const Vec<T, 3>& b) noexcept                \
	{                                                                                \
		return Vec<T, 3>(a op b.x, a op b.y, a op b.z);                              \
	}

#define DEFINE_ARITHMETIC_ASSIGNMENT_OPERATORS(op)                    \
	/* Vector-Vector op */                                            \
	template<typename T>                                              \
	Vec<T, 3>& operator op(Vec<T, 3>& a, const Vec<T, 3>& b) noexcept \
	{                                                                 \
		a.x op b.x;                                                   \
		a.y op b.y;                                                   \
		a.z op b.z;                                                   \
		return a;                                                     \
	}                                                                 \
	/* Vector-Scalar op */                                            \
	template<typename T>                                              \
	Vec<T, 3>& operator op(Vec<T, 3>& a, T b) noexcept                \
	{                                                                 \
		a.x op b;                                                     \
		a.y op b;                                                     \
		a.z op b;                                                     \
		return a;                                                     \
	}

#define DEFINE_COMPARISON_OPERATORS(op)                                                 \
	/* Vector-Vector op */                                                              \
	template<typename T>                                                                \
	constexpr Vec<bool, 3> operator op(const Vec<T, 3>& a, const Vec<T, 3>& b) noexcept \
	{                                                                                   \
		return Vec<bool, 3>(a.x op b.x, a.y op b.y, a.z op b.z);                        \
	}                                                                                   \
	/* Vector-Scalar op */                                                              \
	template<typename T>                                                                \
	constexpr Vec<bool, 3> operator op(const Vec<T, 3>& a, T b) noexcept                \
	{                                                                                   \
		return Vec<bool, 3>(a.x op b, a.y op b, a.z op b);                              \
	}                                                                                   \
	/* Scalar-Vector op */                                                              \
	template<typename T>                                                                \
	constexpr Vec<bool, 3> operator op(T a, const Vec<T, 3>& b) noexcept                \
	{                                                                                   \
		return Vec<bool, 3>(a op b.x, a op b.y, a op b.z);                              \
	}

	DEFINE_UNARY_OPERATOR(-);
	DEFINE_UNARY_OPERATOR(!);
	DEFINE_UNARY_OPERATOR(~);

	DEFINE_BINARY_OPERATORS(+);
	DEFINE_BINARY_OPERATORS(-);
	DEFINE_BINARY_OPERATORS(*);
	DEFINE_BINARY_OPERATORS(/);

	DEFINE_ARITHMETIC_ASSIGNMENT_OPERATORS(+=);
	DEFINE_ARITHMETIC_ASSIGNMENT_OPERATORS(-=);
	DEFINE_ARITHMETIC_ASSIGNMENT_OPERATORS(*=);
	DEFINE_ARITHMETIC_ASSIGNMENT_OPERATORS(/=);

	DEFINE_COMPARISON_OPERATORS(==);
	DEFINE_COMPARISON_OPERATORS(!=);
	DEFINE_COMPARISON_OPERATORS(<);
	DEFINE_COMPARISON_OPERATORS(>);
	DEFINE_COMPARISON_OPERATORS(<=);
	DEFINE_COMPARISON_OPERATORS(>=);

#undef DEFINE_COMPARISON_OPERATORS
#undef DEFINE_ARITHMETIC_ASSIGNMENT_OPERATORS
#undef DEFINE_BINARY_OPERATORS
#undef DEFINE_UNARY_OPERATOR

	// Intrinsics
	template<typename T>
	[[nodiscard]] constexpr T dot(const Vec<T, 3>& a, const Vec<T, 3>& b) noexcept
	{
		return a.x * b.x + a.y * b.y + a.z * b.z;
	}

	template<typename T>
	[[nodiscard]] constexpr T lengthsquared(const Vec<T, 3>& v) noexcept
	{
		return dot(v, v);
	}

	template<typename T>
	[[nodiscard]] constexpr T length(const Vec<T, 3>& v) noexcept
	{
		return sqrt(lengthsquared(v));
	}

	template<typename T>
	[[nodiscard]] Vec<T, 3> normalize(const Vec<T, 3>& v) noexcept
	{
		return v / length(v);
	}

	template<typename T>
	[[nodiscard]] Vec<T, 3> abs(const Vec<T, 3>& v) noexcept
	{
		return Vec<T, 3>(std::abs(v.x), std::abs(v.y), std::abs(v.z));
	}

	template<typename T>
	[[nodiscard]] bool isnan(const Vec<T, 3>& v) noexcept
	{
		return std::isnan(v.x) || std::isnan(v.y) || std::isnan(v.z);
	}

	template<typename T>
	[[nodiscard]] Vec<T, 3> clamp(const Vec<T, 3>& v, T min, T max) noexcept
	{
		return Vec<T, 3>(clamp(v.x, min, max), clamp(v.y, min, max), clamp(v.z, min, max));
	}

	template<typename T>
	[[nodiscard]] Vec<T, 3> saturate(const Vec<T, 3>& v) noexcept
	{
		return clamp(v, T(0), T(1));
	}

	[[nodiscard]] constexpr bool any(const Vec<bool, 3>& v) noexcept
	{
		return v.x || v.y || v.z;
	}

	[[nodiscard]] constexpr bool all(const Vec<bool, 3>& v) noexcept
	{
		return v.x && v.y && v.z;
	}

	template<typename T>
	[[nodiscard]] Vec<T, 3> cross(const Vec<T, 3>& a, const Vec<T, 3>& b) noexcept
	{
		float v1x = a.x, v1y = a.y, v1z = a.z;
		float v2x = b.x, v2y = b.y, v2z = b.z;
		return Vec<T, 3>((v1y * v2z) - (v1z * v2y), (v1z * v2x) - (v1x * v2z), (v1x * v2y) - (v1y * v2x));
	}

	template<typename T>
	[[nodiscard]] Vec<T, 3> faceforward(const Vec<T, 3>& n, const Vec<T, 3>& v) noexcept
	{
		return dot(n, v) < 0.0f ? -n : n;
	}

	using uint3	  = Vec<unsigned, 3>;
	using int3	  = Vec<int, 3>;
	using float3  = Vec<float, 3>;
	using double3 = Vec<double, 3>;
	using bool3	  = Vec<bool, 3>;

	using Vec3u = Vec<unsigned, 3>;
	using Vec3i = Vec<int, 3>;
	using Vec3f = Vec<float, 3>;
	using Vec3d = Vec<double, 3>;
	using Vec3b = Vec<bool, 3>;
} // namespace Math

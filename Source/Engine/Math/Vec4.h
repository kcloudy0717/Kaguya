#pragma once
#include "Vec.h"

namespace Math
{
	template<typename T>
	struct [[nodiscard]] Vec<T, 4>
	{
		constexpr Vec() noexcept
			: x(static_cast<T>(0))
			, y(static_cast<T>(0))
			, z(static_cast<T>(0))
			, w(static_cast<T>(0))
		{
		}
		constexpr Vec(T x, T y, T z, T w) noexcept
			: x(x)
			, y(y)
			, z(z)
			, w(w)
		{
		}
		constexpr Vec(T v) noexcept
			: Vec(v, v, v, v)
		{
		}
		constexpr Vec(const Vec<T, 2>& xy, T z, T w)
			: x(xy.x)
			, y(xy.y)
			, z(z)
			, w(w)
		{
		}
		constexpr Vec(const Vec<T, 2>& xy, const Vec<T, 2>& zw)
			: x(xy.x)
			, y(xy.y)
			, z(zw.x)
			, w(zw.y)
		{
		}
		constexpr Vec(const Vec<T, 3>& xyz, T w)
			: x(xyz.x)
			, y(xyz.y)
			, z(xyz.z)
			, w(w)
		{
		}
		template<typename U>
		constexpr Vec(const Vec<U, 4>& v)
			: x(static_cast<T>(v.x))
			, y(static_cast<T>(v.y))
			, z(static_cast<T>(v.z))
			, w(static_cast<T>(v.w))
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

		T x, y, z, w;

	private:
		static constexpr T Vec::*Array[]{ &Vec::x, &Vec::y, &Vec::z, &Vec::w };
	};

#define DEFINE_UNARY_OPERATOR(op)                                \
	template<typename T>                                         \
	constexpr Vec<T, 4> operator op(const Vec<T, 4>& a) noexcept \
	{                                                            \
		return Vec<T, 4>(op a.x, op a.y, op a.z, op a.w);        \
	}

#define DEFINE_BINARY_OPERATORS(op)                                                  \
	/* Vector-Vector op */                                                           \
	template<typename T>                                                             \
	constexpr Vec<T, 4> operator op(const Vec<T, 4>& a, const Vec<T, 4>& b) noexcept \
	{                                                                                \
		return Vec<T, 4>(a.x op b.x, a.y op b.y, a.z op b.z, a.w op b.w);            \
	}                                                                                \
	/* Vector-Scalar op */                                                           \
	template<typename T>                                                             \
	constexpr Vec<T, 4> operator op(const Vec<T, 4>& a, T b) noexcept                \
	{                                                                                \
		return Vec<T, 4>(a.x op b, a.y op b, a.z op b, a.w op b);                    \
	}                                                                                \
	/* Scalar-Vector op */                                                           \
	template<typename T>                                                             \
	constexpr Vec<T, 4> operator op(T a, const Vec<T, 4>& b) noexcept                \
	{                                                                                \
		return Vec<T, 4>(a op b.x, a op b.y, a op b.z, a op b.w);                    \
	}

#define DEFINE_ARITHMETIC_ASSIGNMENT_OPERATORS(op)                    \
	/* Vector-Vector op */                                            \
	template<typename T>                                              \
	Vec<T, 4>& operator op(Vec<T, 4>& a, const Vec<T, 4>& b) noexcept \
	{                                                                 \
		a.x op b.x;                                                   \
		a.y op b.y;                                                   \
		a.z op b.z;                                                   \
		a.w op b.w;                                                   \
		return a;                                                     \
	}                                                                 \
	/* Vector-Scalar op */                                            \
	template<typename T>                                              \
	Vec<T, 4>& operator op(Vec<T, 4>& a, T b) noexcept                \
	{                                                                 \
		a.x op b;                                                     \
		a.y op b;                                                     \
		a.z op b;                                                     \
		a.w op b;                                                     \
		return a;                                                     \
	}

#define DEFINE_COMPARISON_OPERATORS(op)                                                 \
	/* Vector-Vector op */                                                              \
	template<typename T>                                                                \
	constexpr Vec<bool, 4> operator op(const Vec<T, 4>& a, const Vec<T, 4>& b) noexcept \
	{                                                                                   \
		return Vec<bool, 4>(a.x op b.x, a.y op b.y, a.z op b.z, a.w op b.w);            \
	}                                                                                   \
	/* Vector-Scalar op */                                                              \
	template<typename T>                                                                \
	constexpr Vec<bool, 4> operator op(const Vec<T, 4>& a, T b) noexcept                \
	{                                                                                   \
		return Vec<bool, 4>(a.x op b, a.y op b, a.z op b, a.w op b);                    \
	}                                                                                   \
	/* Scalar-Vector op */                                                              \
	template<typename T>                                                                \
	constexpr Vec<bool, 4> operator op(T a, const Vec<T, 4>& b) noexcept                \
	{                                                                                   \
		return Vec<bool, 4>(a op b.x, a op b.y, a op b.z, a op b.w);                    \
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
	[[nodiscard]] constexpr T dot(const Vec<T, 4>& a, const Vec<T, 4>& b) noexcept
	{
		return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
	}

	template<typename T>
	[[nodiscard]] constexpr T lengthsquared(const Vec<T, 4>& v) noexcept
	{
		return dot(v, v);
	}

	template<typename T>
	[[nodiscard]] constexpr T length(const Vec<T, 4>& v) noexcept
	{
		return sqrt(lengthsquared(v));
	}

	template<typename T>
	[[nodiscard]] Vec<T, 4> normalize(const Vec<T, 4>& v) noexcept
	{
		return v / length(v);
	}

	template<typename T>
	[[nodiscard]] Vec<T, 4> abs(const Vec<T, 4>& v) noexcept
	{
		return Vec<T, 4>(std::abs(v.x), std::abs(v.y), std::abs(v.z), std::abs(v.w));
	}

	template<typename T>
	[[nodiscard]] bool isnan(const Vec<T, 4>& v) noexcept
	{
		return std::isnan(v.x) || std::isnan(v.y) || std::isnan(v.z) || std::isnan(v.w);
	}

	template<typename T>
	[[nodiscard]] Vec<T, 4> clamp(const Vec<T, 4>& v, T min, T max) noexcept
	{
		return Vec<T, 4>(clamp(v.x, min, max), clamp(v.y, min, max), clamp(v.z, min, max), clamp(v.w, min, max));
	}

	template<typename T>
	[[nodiscard]] Vec<T, 4> saturate(const Vec<T, 4>& v) noexcept
	{
		return clamp(v, T(0), T(1));
	}

	[[nodiscard]] constexpr bool any(const Vec<bool, 4>& v) noexcept
	{
		return v.x || v.y || v.z || v.w;
	}

	[[nodiscard]] constexpr bool all(const Vec<bool, 4>& v) noexcept
	{
		return v.x && v.y && v.z && v.w;
	}

	using Vec4i = Vec<int, 4>;
	using Vec4f = Vec<float, 4>;
	using Vec4d = Vec<double, 4>;
	using Vec4b = Vec<bool, 4>;
} // namespace Math

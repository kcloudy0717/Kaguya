#pragma once
#include "Vec.h"

namespace Math
{
	template<typename T>
	struct [[nodiscard]] Vec<T, 2>
	{
		constexpr Vec() noexcept
			: x(static_cast<T>(0))
			, y(static_cast<T>(0))
		{
		}
		constexpr Vec(T x, T y) noexcept
			: x(x)
			, y(y)
		{
		}
		constexpr Vec(T v) noexcept
			: Vec(v, v)
		{
		}
		constexpr Vec(const Vec<T, 3>& v) noexcept
			: x(v.x)
			, y(v.y)
		{
		}
		constexpr Vec(const Vec<T, 4>& v) noexcept
			: x(v.x)
			, y(v.y)
		{
		}
		template<typename U>
		constexpr Vec(const Vec<U, 2>& v) noexcept
			: x(static_cast<T>(v.x))
			, y(static_cast<T>(v.y))
		{
		}

		T*		 data() noexcept { return reinterpret_cast<T*>(this); }
		const T* data() const noexcept { return reinterpret_cast<const T*>(this); }

		T operator[](size_t i) const noexcept
		{
			return this->*Array[i];
		}

		T& operator[](size_t i) noexcept
		{
			return this->*Array[i];
		}

		T x, y;

	private:
		static constexpr T Vec::*Array[]{ &Vec::x, &Vec::y };
	};

#define DEFINE_UNARY_OPERATOR(op)                                \
	template<typename T>                                         \
	constexpr Vec<T, 2> operator op(const Vec<T, 2>& a) noexcept \
	{                                                            \
		return Vec<T, 2>(op a.x, op a.y);                        \
	}

#define DEFINE_BINARY_OPERATORS(op)                                                  \
	/* Vector-Vector op */                                                           \
	template<typename T>                                                             \
	constexpr Vec<T, 2> operator op(const Vec<T, 2>& a, const Vec<T, 2>& b) noexcept \
	{                                                                                \
		return Vec<T, 2>(a.x op b.x, a.y op b.y);                                    \
	}                                                                                \
	/* Vector-Scalar op */                                                           \
	template<typename T>                                                             \
	constexpr Vec<T, 2> operator op(const Vec<T, 2>& a, T b) noexcept                \
	{                                                                                \
		return Vec<T, 2>(a.x op b, a.y op b);                                        \
	}                                                                                \
	/* Scalar-Vector op */                                                           \
	template<typename T>                                                             \
	constexpr Vec<T, 2> operator op(T a, const Vec<T, 2>& b) noexcept                \
	{                                                                                \
		return Vec<T, 2>(a op b.x, a op b.y);                                        \
	}

#define DEFINE_ARITHMETIC_ASSIGNMENT_OPERATORS(op)                    \
	/* Vector-Vector op */                                            \
	template<typename T>                                              \
	Vec<T, 2>& operator op(Vec<T, 2>& a, const Vec<T, 2>& b) noexcept \
	{                                                                 \
		a.x op b.x;                                                   \
		a.y op b.y;                                                   \
		return a;                                                     \
	}                                                                 \
	/* Vector-Scalar op */                                            \
	template<typename T>                                              \
	Vec<T, 2>& operator op(Vec<T, 2>& a, T b) noexcept                \
	{                                                                 \
		a.x op b;                                                     \
		a.y op b;                                                     \
		return a;                                                     \
	}

#define DEFINE_COMPARISON_OPERATORS(op)                                                 \
	/* Vector-Vector op */                                                              \
	template<typename T>                                                                \
	constexpr Vec<bool, 2> operator op(const Vec<T, 2>& a, const Vec<T, 2>& b) noexcept \
	{                                                                                   \
		return Vec<bool, 2>(a.x op b.x, a.y op b.y);                                    \
	}                                                                                   \
	/* Vector-Scalar op */                                                              \
	template<typename T>                                                                \
	constexpr Vec<bool, 2> operator op(const Vec<T, 2>& a, T b) noexcept                \
	{                                                                                   \
		return Vec<bool, 2>(a.x op b, a.y op b);                                        \
	}                                                                                   \
	/* Scalar-Vector op */                                                              \
	template<typename T>                                                                \
	constexpr Vec<bool, 2> operator op(T a, const Vec<T, 2>& b) noexcept                \
	{                                                                                   \
		return Vec<bool, 2>(a op b.x, a op b.y);                                        \
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
	[[nodiscard]] constexpr T dot(const Vec<T, 2>& a, const Vec<T, 2>& b) noexcept
	{
		return a.x * b.x + a.y * b.y;
	}

	template<typename T>
	[[nodiscard]] constexpr T lengthsquared(const Vec<T, 2>& v) noexcept
	{
		return dot(v, v);
	}

	template<typename T>
	[[nodiscard]] constexpr T length(const Vec<T, 2>& v) noexcept
	{
		return sqrt(lengthsquared(v));
	}

	template<typename T>
	[[nodiscard]] Vec<T, 2> normalize(const Vec<T, 2>& v) noexcept
	{
		return v / length(v);
	}

	template<typename T>
	[[nodiscard]] Vec<T, 2> abs(const Vec<T, 2>& v) noexcept
	{
		return Vec<T, 2>(std::abs(v.x), std::abs(v.y));
	}

	template<typename T>
	[[nodiscard]] bool isnan(const Vec<T, 2>& v) noexcept
	{
		return std::isnan(v.x) || std::isnan(v.y);
	}

	template<typename T>
	[[nodiscard]] Vec<T, 2> clamp(const Vec<T, 2>& v, T min, T max) noexcept
	{
		return Vec2(clamp(v.x, min, max), clamp(v.y, min, max));
	}

	template<typename T>
	[[nodiscard]] Vec<T, 2> saturate(const Vec<T, 2>& v) noexcept
	{
		return clamp(v, T(0), T(1));
	}

	[[nodiscard]] constexpr bool any(const Vec<bool, 2>& v) noexcept
	{
		return v.x || v.y;
	}

	[[nodiscard]] constexpr bool all(const Vec<bool, 2>& v) noexcept
	{
		return v.x && v.y;
	}

	using Vec2u = Vec<unsigned, 2>;
	using Vec2i = Vec<int, 2>;
	using Vec2f = Vec<float, 2>;
	using Vec2d = Vec<double, 2>;
	using Vec2b = Vec<bool, 2>;
} // namespace Math

#pragma once

namespace Math
{
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

		// Imaginary part
		float x;
		float y;
		float z;
		// Real part
		float w;
	};

#define DEFINE_BINARY_OPERATORS(op)                                            \
	/* Quaternion-Quaternion op */                                             \
	constexpr Quaternion operator op(const Quaternion& a, const Quaternion& b) \
	{                                                                          \
		return Quaternion(a.x op b.x, a.y op b.y, a.z op b.z, a.w op b.w);     \
	}                                                                          \
	/* Quaternion-Scalar op */                                                 \
	constexpr Quaternion operator op(const Quaternion& a, float b)             \
	{                                                                          \
		return Quaternion(a.x op b, a.y op b, a.z op b, a.w op b);             \
	}                                                                          \
	/* Scalar-Quaternion op */                                                 \
	constexpr Quaternion operator op(float a, const Quaternion& b)             \
	{                                                                          \
		return Quaternion(a op b.x, a op b.y, a op b.z, a op b.w);             \
	}

	DEFINE_BINARY_OPERATORS(+);
	DEFINE_BINARY_OPERATORS(-);

#undef DEFINE_BINARY_OPERATORS

	// Intrinsics
	[[nodiscard]] inline float dot(const Quaternion& a, const Quaternion& b) noexcept
	{
		return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
	}

	[[nodiscard]] inline float lengthsquared(const Quaternion& v) noexcept
	{
		return dot(v, v);
	}

	[[nodiscard]] inline float length(const Quaternion& v) noexcept
	{
		return sqrt(lengthsquared(v));
	}

	[[nodiscard]] inline Quaternion normalize(const Quaternion& v) noexcept
	{
		float s = 1.0f / length(v);
		return { v.x * s, v.y * s, v.z * s, v.w * s };
	}

	[[nodiscard]] inline Quaternion conjugate(const Quaternion& q) noexcept
	{
		return { -q.x, -q.y, -q.z, q.w };
	}

	[[nodiscard]] inline Quaternion inverse(const Quaternion& q) noexcept
	{
		// q* x q x q^-1 = q*
		// |q|^2 x q^-1 = q*
		// q^-1 = q* / |q|^2
		Quaternion c = conjugate(q);
		float	   s = 1.0f / lengthsquared(q);
		return { c.x * s, c.y * s, c.z * s, c.w * s };
	}

} // namespace Math

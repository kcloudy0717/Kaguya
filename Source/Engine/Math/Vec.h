#pragma once
#include <cmath>

namespace Math
{
	// Base N dimensional vector struct
	template<typename T, size_t N>
	struct [[nodiscard]] Vec
	{
	};

	[[nodiscard]] constexpr float lerp(float x, float y, float s) noexcept
	{
		return x + s * (y - x);
	}

	template<typename T>
	[[nodiscard]] constexpr T clamp(T value, T min, T max) noexcept
	{
		return std::min(std::max(value, min), max);
	}
	template<typename T>
	[[nodiscard]] constexpr T saturate(T value) noexcept
	{
		return clamp(value, static_cast<T>(0), static_cast<T>(1));
	}
} // namespace Math

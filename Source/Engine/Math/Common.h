#pragma once

template<typename T, typename U>
constexpr T AlignUp(T Size, U Alignment)
{
	return (T)(((size_t)Size + (size_t)Alignment - 1) & ~((size_t)Alignment - 1));
}

template<typename T, typename U>
constexpr T AlignDown(T Size, U Alignment)
{
	return (T)((size_t)Size & ~((size_t)Alignment - 1));
}

template<typename T>
constexpr T RoundUpAndDivide(T Value, size_t Alignment)
{
	return (T)((Value + Alignment - 1) / Alignment);
}

template<typename T>
constexpr bool IsPowerOfTwo(T Value)
{
	return 0 == (Value & (Value - 1));
}

template<typename T>
constexpr bool IsDivisible(T Value, T Divisor)
{
	return (Value / Divisor) * Divisor == Value;
}

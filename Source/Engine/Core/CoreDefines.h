#pragma once

#define DEFAULTCOPYABLE(TypeName)                                                                                      \
	TypeName(const TypeName&) = default;                                                                               \
	TypeName& operator=(const TypeName&) = default

#define DEFAULTMOVABLE(TypeName)                                                                                       \
	TypeName(TypeName&&) noexcept = default;                                                                           \
	TypeName& operator=(TypeName&&) noexcept = default

#define NONCOPYABLE(TypeName)                                                                                          \
	TypeName(const TypeName&) = delete;                                                                                \
	TypeName& operator=(const TypeName&) = delete

#define NONMOVABLE(TypeName)                                                                                           \
	TypeName(TypeName&&) noexcept = delete;                                                                            \
	TypeName& operator=(TypeName&&) noexcept = delete

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

inline std::uint8_t Log2(std::uint64_t Value)
{
	unsigned long mssb; // most significant set bit
	unsigned long lssb; // least significant set bit

	// If perfect power of two (only one set bit), return index of bit.  Otherwise round up
	// fractional log by adding 1 to most signicant set bit's index.
	if (_BitScanReverse64(&mssb, Value) > 0 && _BitScanForward64(&lssb, Value) > 0)
		return std::uint8_t(mssb + (mssb == lssb ? 0 : 1));

	return 0;
}

constexpr std::size_t operator"" _KiB(std::size_t X)
{
	return X * 1024;
}

constexpr std::size_t operator"" _MiB(std::size_t X)
{
	return X * 1024 * 1024;
}

constexpr std::size_t operator"" _GiB(std::size_t X)
{
	return X * 1024 * 1024 * 1024;
}

constexpr std::size_t ToKiB(std::size_t Byte)
{
	return Byte / 1024;
}

constexpr std::size_t ToMiB(std::size_t Byte)
{
	return Byte / 1024 / 1024;
}

constexpr std::size_t ToGiB(std::size_t Byte)
{
	return Byte / 1024 / 1024 / 1024;
}

#include <type_traits>
// http://blog.bitwigglers.org/using-enum-classes-as-type-safe-bitmasks/

template<typename Enum>
struct EnableBitMaskOperators
{
	static constexpr bool Enable = false;
};

template<typename Enum>
constexpr std::enable_if_t<EnableBitMaskOperators<Enum>::Enable, Enum> operator|(Enum Lhs, Enum Rhs)
{
	using UnderlyingType = std::underlying_type_t<Enum>;
	return static_cast<Enum>(static_cast<UnderlyingType>(Lhs) | static_cast<UnderlyingType>(Rhs));
}

template<typename Enum>
constexpr std::enable_if_t<EnableBitMaskOperators<Enum>::Enable, Enum> operator&(Enum Lhs, Enum Rhs)
{
	using UnderlyingType = std::underlying_type_t<Enum>;
	return static_cast<Enum>(static_cast<UnderlyingType>(Lhs) & static_cast<UnderlyingType>(Rhs));
}

template<typename Enum>
constexpr std::enable_if_t<EnableBitMaskOperators<Enum>::Enable, Enum> operator^(Enum Lhs, Enum Rhs)
{
	using UnderlyingType = std::underlying_type_t<Enum>;
	return static_cast<Enum>(static_cast<UnderlyingType>(Lhs) ^ static_cast<UnderlyingType>(Rhs));
}

template<typename Enum>
constexpr std::enable_if_t<EnableBitMaskOperators<Enum>::Enable, Enum> operator~(Enum Rhs)
{
	using UnderlyingType = std::underlying_type_t<Enum>;
	return static_cast<Enum>(~static_cast<UnderlyingType>(Rhs));
}

template<typename Enum>
constexpr std::enable_if_t<EnableBitMaskOperators<Enum>::Enable, Enum> operator|=(Enum& Lhs, Enum Rhs)
{
	using UnderlyingType = std::underlying_type_t<Enum>;
	Lhs					 = static_cast<Enum>(static_cast<UnderlyingType>(Lhs) | static_cast<UnderlyingType>(Rhs));
	return Lhs;
}

template<typename Enum>
constexpr std::enable_if_t<EnableBitMaskOperators<Enum>::Enable, Enum> operator&=(Enum& Lhs, Enum Rhs)
{
	using UnderlyingType = std::underlying_type_t<Enum>;
	Lhs					 = static_cast<Enum>(static_cast<UnderlyingType>(Lhs) & static_cast<UnderlyingType>(Rhs));
	return Lhs;
}

template<typename Enum>
constexpr std::enable_if_t<EnableBitMaskOperators<Enum>::Enable, Enum> operator^=(Enum& Lhs, Enum Rhs)
{
	using UnderlyingType = std::underlying_type_t<Enum>;
	Lhs					 = static_cast<Enum>(static_cast<UnderlyingType>(Lhs) ^ static_cast<UnderlyingType>(Rhs));
	return Lhs;
}

#define ENABLE_BITMASK_OPERATORS(Enum)                                                                                 \
	template<>                                                                                                         \
	struct EnableBitMaskOperators<Enum>                                                                                \
	{                                                                                                                  \
		static constexpr bool Enable = true;                                                                           \
	};                                                                                                                 \
                                                                                                                       \
	constexpr bool EnumMaskBitSet(Enum Mask, Enum Component) { return (Mask & Component) == Component; }

// http://reedbeta.com/blog/python-like-enumerate-in-cpp17/
#include <tuple>

template<
	typename T,
	typename TIter = decltype(std::begin(std::declval<T>())),
	typename	   = decltype(std::end(std::declval<T>()))>
constexpr auto enumerate(T&& iterable)
{
	struct iterator
	{
		size_t i;
		TIter  iter;
		bool   operator!=(const iterator& other) const { return iter != other.iter; }
		void   operator++()
		{
			++i;
			++iter;
		}
		auto operator*() const { return std::tie(i, *iter); }
	};
	struct iterable_wrapper
	{
		T	 iterable;
		auto begin() { return iterator{ 0, std::begin(iterable) }; }
		auto end() { return iterator{ 0, std::end(iterable) }; }
	};
	return iterable_wrapper{ std::forward<T>(iterable) };
}

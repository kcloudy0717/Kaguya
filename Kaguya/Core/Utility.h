#pragma once

#include <type_traits>
// http://blog.bitwigglers.org/using-enum-classes-as-type-safe-bitmasks/

template<typename Enum>
struct EnableBitMaskOperators
{
	static const bool Enable = false;
};

template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::Enable, Enum>::type
operator|(Enum lhs, Enum rhs)
{
	using underlying = typename std::underlying_type<Enum>::type;
	return static_cast<Enum>(static_cast<underlying>(lhs) | static_cast<underlying>(rhs));
}

template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::Enable, Enum>::type
operator&(Enum lhs, Enum rhs)
{
	using underlying = typename std::underlying_type<Enum>::type;
	return static_cast<Enum>(static_cast<underlying>(lhs) & static_cast<underlying>(rhs));
}

template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::Enable, Enum>::type
operator^(Enum lhs, Enum rhs)
{
	using underlying = typename std::underlying_type<Enum>::type;
	return static_cast<Enum>(static_cast<underlying>(lhs) ^ static_cast<underlying>(rhs));
}

template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::Enable, Enum>::type
operator~(Enum rhs)
{
	using underlying = typename std::underlying_type<Enum>::type;
	return static_cast<Enum>(~static_cast<underlying>(rhs));
}

template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::Enable, Enum&>::type
operator|=(Enum& lhs, Enum rhs)
{
	using underlying = typename std::underlying_type<Enum>::type;
	lhs = static_cast<Enum>(static_cast<underlying>(lhs) | static_cast<underlying>(rhs));
	return lhs;
}

template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::Enable, Enum&>::type
operator&=(Enum& lhs, Enum rhs)
{
	using underlying = typename std::underlying_type<Enum>::type;
	lhs = static_cast<Enum>(static_cast<underlying>(lhs) & static_cast<underlying>(rhs));
	return lhs;
}

template<typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::Enable, Enum&>::type
operator^=(Enum& lhs, Enum rhs)
{
	using underlying = typename std::underlying_type<Enum>::type;
	lhs = static_cast<Enum>(static_cast<underlying>(lhs) ^ static_cast<underlying>(rhs));
	return lhs;
}

#define ENABLE_BITMASK_OPERATORS(Enum)						\
template<>													\
struct EnableBitMaskOperators<Enum>							\
{															\
	static const bool Enable = true;						\
};															\
															\
inline bool EnumMaskBitSet(Enum Mask, Enum Component)		\
{															\
    return (Mask & Component) == Component;					\
}

#include <codecvt>
#include <string>

inline std::wstring UTF8ToUTF16(const std::string& utf8Str)
{
	static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
	return conv.from_bytes(utf8Str);
}

inline std::string UTF16ToUTF8(const std::wstring& utf16Str)
{
	static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
	return conv.to_bytes(utf16Str);
}

// https://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x
template <typename T>
inline void hash_combine(std::size_t& seed, const T& v)
{
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

// http://reedbeta.com/blog/python-like-enumerate-in-cpp17/
#include <tuple>

template <typename T,
	typename TIter = decltype(std::begin(std::declval<T>())),
	typename = decltype(std::end(std::declval<T>()))>
	constexpr auto enumerate(T&& iterable)
{
	struct iterator
	{
		size_t i;
		TIter iter;
		bool operator != (const iterator& other) const { return iter != other.iter; }
		void operator ++ () { ++i; ++iter; }
		auto operator * () const { return std::tie(i, *iter); }
	};
	struct iterable_wrapper
	{
		T iterable;
		auto begin() { return iterator{ 0, std::begin(iterable) }; }
		auto end() { return iterator{ 0, std::end(iterable) }; }
	};
	return iterable_wrapper{ std::forward<T>(iterable) };
}
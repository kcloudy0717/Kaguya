#pragma once
#include <type_traits>

using i8  = signed char;
using i16 = short;
using i32 = int;
using i64 = long long;

using u8  = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;

using f32 = float;
using f64 = double;

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

#define ENABLE_BITMASK_OPERATORS(Enum)       \
	template<>                               \
	struct EnableBitMaskOperators<Enum>      \
	{                                        \
		static constexpr bool Enable = true; \
	};                                       \
                                             \
	constexpr bool EnumMaskBitSet(Enum Mask, Enum Component) { return (Mask & Component) == Component; }

enum class FileMode
{
	Unknown,

	// Specifies that the operating system should create a new file. This requires Write permission. If the file already
	// exists, an IOException exception is thrown.
	CreateNew,

	// Specifies that the operating system should create a new file. If the file already exists, it will be overwritten.
	// This requires Write permission. FileMode.Create is equivalent to requesting that if the file does not exist, use
	// CreateNew; otherwise, use Truncate. If the file already exists but is a hidden file, an
	// UnauthorizedAccessException exception is thrown.
	Create,

	// Specifies that the operating system should open an existing file. The ability to open the file is dependent on
	// the value specified by the FileAccess enumeration. A FileNotFoundException exception is thrown if the file does
	// not exist.
	Open,

	// Specifies that the operating system should open a file if it exists; otherwise, a new file should be created. If
	// the file is opened with FileAccess.Read, Read permission is required. If the file access is FileAccess.Write,
	// Write permission is required. If the file is opened with FileAccess.ReadWrite, both Read and Write permissions
	// are required.
	OpenOrCreate,

	// Specifies that the operating system should open an existing file. When the file is opened, it should be truncated
	// so that its size is zero bytes. This requires Write permission. Attempts to read from a file opened with
	// FileMode.Truncate cause an ArgumentException exception.
	Truncate,
};

enum class FileAccess
{
	Unknown,
	Read,
	Write,
	ReadWrite
};

enum class SeekOrigin
{
	Begin,
	Current,
	End
};

enum class NotifyFilters
{
	None	   = 0,
	FileName   = 1 << 0,
	DirName	   = 1 << 1,
	Attributes = 1 << 2,
	Size	   = 1 << 3,
	LastWrite  = 1 << 4,
	LastAccess = 1 << 5,
	Creation   = 1 << 6,
	Security   = 1 << 7,

	All = 0xff
};
ENABLE_BITMASK_OPERATORS(NotifyFilters);

#pragma once
#include <filesystem>
#include <wil/resource.h>
#include <type_traits>

// clang-format off
// Decimal SI units
constexpr size_t operator""_KB(size_t x) { return x * 1000; }
constexpr size_t operator""_MB(size_t x) { return x * 1000 * 1000; }
constexpr size_t operator""_GB(size_t x) { return x * 1000 * 1000 * 1000; }

// Binary SI units
constexpr size_t operator""_KiB(size_t x) { return x * 1024; }
constexpr size_t operator""_MiB(size_t x) { return x * 1024 * 1024; }
constexpr size_t operator""_GiB(size_t x) { return x * 1024 * 1024 * 1024; }
// clang-format on

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

using i8  = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using u8  = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using f32 = std::float_t;
using f64 = std::double_t;

enum class FileMode
{
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

	All = 0xFF
};
ENABLE_BITMASK_OPERATORS(NotifyFilters);

struct FileSystemEventArgs
{
	std::filesystem::path Path;
};

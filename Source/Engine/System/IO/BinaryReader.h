#pragma once
#include <cassert>
#include <memory>
#include "Types.h"

class FileStream;

class BinaryReader
{
public:
	explicit BinaryReader(FileStream& Stream);

	void Read(void* DstData, u64 SizeInBytes) const noexcept;

	template<typename T>
	T Read() const noexcept
	{
		static_assert(std::is_trivial_v<T>, "typename T is not trivial");
		assert(Ptr + sizeof(T) <= Sentinel);

		std::byte* Data = Ptr;
		Ptr += sizeof(T);
		return *reinterpret_cast<T*>(Data);
	}

private:
	FileStream&					 Stream;
	std::unique_ptr<std::byte[]> BaseAddress;
	mutable std::byte*			 Ptr;
	std::byte*					 Sentinel;
};

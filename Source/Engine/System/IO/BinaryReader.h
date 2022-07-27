#pragma once
#include <memory>
#include "Types.h"

class FileStream;

class BinaryReader
{
public:
	explicit BinaryReader(FileStream& Stream);

	u8* GetData() const noexcept;

	void Read(void* DstData, u64 SizeInBytes) const noexcept;

	template<typename T>
	T Read() const noexcept
	{
		static_assert(std::is_trivial_v<T>, "typename T is not trivial");
		T Result = {};
		Read(&Result, sizeof(T));
		return Result;
	}

	u8	ReadByte() const noexcept;
	// Lifetime of the returned pointer is tied to the BinaryReader for efficiency reasons.
	u8* ReadBytes(u64 SizeInBytes) const noexcept;

private:
	FileStream&			  Stream;
	std::unique_ptr<u8[]> BaseAddress;
	mutable u8*			  Ptr;
	u8*					  Sentinel;
};

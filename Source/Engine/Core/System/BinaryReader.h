#pragma once
#include "FileStream.h"

class BinaryReader
{
public:
	explicit BinaryReader(FileStream& Stream);

	void Read(void* DstData, size_t SizeInBytes) const
	{
		assert(Ptr + SizeInBytes <= Sentinel);

		BYTE* SrcData = Ptr;
		Ptr += SizeInBytes;
		std::memcpy(DstData, SrcData, SizeInBytes);
	}

	template<typename T>
	T Read() const
	{
		static_assert(std::is_trivial_v<T>, "typename T is not trivial");

		assert(Ptr + sizeof(T) <= Sentinel);
		BYTE* Data = Ptr;
		Ptr += sizeof(T);
		return *reinterpret_cast<T*>(Data);
	}

private:
	FileStream&				Stream;
	std::unique_ptr<BYTE[]> BaseAddress;
	mutable BYTE*			Ptr;
	BYTE*					Sentinel;
};

#include "BinaryReader.h"
#include "FileStream.h"
#include <cassert>

BinaryReader::BinaryReader(FileStream& Stream)
	: Stream(Stream)
{
	assert(Stream.CanRead());
	BaseAddress = Stream.ReadAll();
	Ptr			= BaseAddress.get();
	Sentinel	= Ptr + Stream.GetSizeInBytes();
}

u8* BinaryReader::GetBaseAddress() const noexcept
{
	return BaseAddress.get();
}

u8* BinaryReader::GetPtr() const noexcept
{
	return Ptr;
}

usize BinaryReader::GetSizeInBytes() const noexcept
{
	return Stream.GetSizeInBytes();
}

void BinaryReader::Read(void* DstData, u64 SizeInBytes) const noexcept
{
	assert(Ptr + SizeInBytes <= Sentinel);
	u8* SrcData = Ptr;
	Ptr += SizeInBytes;
	std::memcpy(DstData, SrcData, SizeInBytes);
}

u8 BinaryReader::ReadByte() const noexcept
{
	assert(Ptr + 1 <= Sentinel);
	return *Ptr++;
}

u8* BinaryReader::ReadBytes(u64 SizeInBytes) const noexcept
{
	assert(Ptr + SizeInBytes <= Sentinel);
	u8* SrcData = Ptr;
	Ptr += SizeInBytes;
	return SrcData;
}

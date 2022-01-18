#include "BinaryReader.h"
#include "FileStream.h"

BinaryReader::BinaryReader(FileStream& Stream)
	: Stream(Stream)
{
	assert(Stream.CanRead());
	BaseAddress = Stream.ReadAll();
	Ptr			= BaseAddress.get();
	Sentinel	= Ptr + Stream.GetSizeInBytes();
}

void BinaryReader::Read(void* DstData, u64 SizeInBytes) const noexcept
{
	assert(Ptr + SizeInBytes <= Sentinel);

	std::byte* SrcData = Ptr;
	Ptr += SizeInBytes;
	std::memcpy(DstData, SrcData, SizeInBytes);
}

#include "BinaryReader.h"

BinaryReader::BinaryReader(FileStream& Stream)
	: Stream(Stream)
{
	assert(Stream.CanRead());
	BaseAddress = Stream.ReadAll();
	Ptr			= BaseAddress.get();
	Sentinel	= Ptr + Stream.GetSizeInBytes();
}

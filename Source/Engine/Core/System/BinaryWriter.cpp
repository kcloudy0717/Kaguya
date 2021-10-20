#include "BinaryWriter.h"

BinaryWriter::BinaryWriter(FileStream& Stream)
	: Stream(Stream)
{
	assert(Stream.CanWrite());
}

void BinaryWriter::Write(const void* Data, size_t SizeInBytes) const
{
	Stream.Write(Data, SizeInBytes);
}

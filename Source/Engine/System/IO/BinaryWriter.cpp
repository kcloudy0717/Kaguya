#include "BinaryWriter.h"
#include "FileStream.h"

BinaryWriter::BinaryWriter(FileStream& Stream)
	: Stream(Stream)
{
	assert(Stream.CanWrite());
}

void BinaryWriter::Write(const void* Data, u64 SizeInBytes) const
{
	Stream.Write(Data, SizeInBytes);
}

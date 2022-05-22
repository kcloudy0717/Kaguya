#include "BinaryWriter.h"
#include <cassert>
#include "FileStream.h"

BinaryWriter::BinaryWriter(FileStream& Stream)
	: Stream(Stream)
{
	assert(Stream.CanWrite());
}

void BinaryWriter::Write(const void* Data, u64 SizeInBytes) const
{
	u64 BytesWritten = Stream.Write(Data, SizeInBytes);
	assert(BytesWritten == SizeInBytes);
}

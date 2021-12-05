#include "MemoryMappedFile.h"
#include <iostream>

MemoryMappedFile::MemoryMappedFile(FileStream& Stream, UINT64 FileSize /*= DefaultFileSize*/)
	: Stream(Stream)
{
	InternalCreate(FileSize);
}

MemoryMappedFile::~MemoryMappedFile()
{
}

MemoryMappedView MemoryMappedFile::CreateView()
{
	constexpr DWORD DesiredAccess = FILE_MAP_ALL_ACCESS;
	LPVOID			View		  = MapViewOfFile(FileMapping.get(), DesiredAccess, 0, 0, CurrentFileSize);
	return MemoryMappedView(static_cast<BYTE*>(View), CurrentFileSize);
}

MemoryMappedView MemoryMappedFile::CreateView(UINT Offset, UINT64 SizeInBytes)
{
	constexpr DWORD DesiredAccess = FILE_MAP_ALL_ACCESS;
	LPVOID			View		  = MapViewOfFile(FileMapping.get(), DesiredAccess, 0, Offset, SizeInBytes);
	return MemoryMappedView(static_cast<BYTE*>(View), SizeInBytes);
}

void MemoryMappedFile::GrowMapping(UINT64 Size)
{
	// Check the size.
	if (Size <= CurrentFileSize)
	{
		// Don't shrink.
		return;
	}

	// Update the size and create a new mapping.
	InternalCreate(Size);
}

void MemoryMappedFile::InternalCreate(UINT64 FileSize)
{
	CurrentFileSize = Stream.GetSizeInBytes();
	if (CurrentFileSize == 0)
	{
		// File mapping files with a size of 0 produces an error.
		CurrentFileSize = DefaultFileSize;
	}
	else if (FileSize > CurrentFileSize)
	{
		// Grow to the specified size.
		CurrentFileSize = FileSize;
	}

	FileMapping.reset(CreateFileMapping(Stream.GetHandle(), nullptr, PAGE_READWRITE, 0, CurrentFileSize, nullptr));
	if (!FileMapping)
	{
		ErrorExit(__FUNCTIONW__);
	}
}

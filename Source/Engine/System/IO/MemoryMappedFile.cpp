#include "MemoryMappedFile.h"
#include "FileStream.h"

MemoryMappedFile::MemoryMappedFile(
	FileStream& Stream,
	u64			FileSize /*= DefaultFileSize*/)
	: Stream(Stream)
{
	InternalCreate(FileSize);
}

u64 MemoryMappedFile::GetCurrentFileSize() const noexcept
{
	return CurrentFileSize;
}

MemoryMappedView MemoryMappedFile::CreateView() const noexcept
{
	constexpr DWORD DesiredAccess = FILE_MAP_ALL_ACCESS;
	LPVOID			View		  = MapViewOfFile(FileMapping.get(), DesiredAccess, 0, 0, CurrentFileSize);
	return MemoryMappedView(static_cast<std::byte*>(View), CurrentFileSize);
}

MemoryMappedView MemoryMappedFile::CreateView(u32 Offset, u64 SizeInBytes) const noexcept
{
	constexpr DWORD DesiredAccess = FILE_MAP_ALL_ACCESS;
	LPVOID			View		  = MapViewOfFile(FileMapping.get(), DesiredAccess, 0, Offset, SizeInBytes);
	return MemoryMappedView(static_cast<std::byte*>(View), SizeInBytes);
}

void MemoryMappedFile::GrowMapping(u64 Size)
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

void MemoryMappedFile::InternalCreate(u64 FileSize)
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

	FileMapping.reset(CreateFileMapping(
		Stream.GetHandle(),
		nullptr,
		PAGE_READWRITE,
		0,
		static_cast<DWORD>(CurrentFileSize),
		nullptr));
	if (!FileMapping)
	{
		ErrorExit(__FUNCTIONW__);
	}
}

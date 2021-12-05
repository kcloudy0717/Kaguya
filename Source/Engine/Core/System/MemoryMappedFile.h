#pragma once
#include "FileMode.h"
#include "FileStream.h"
#include "MemoryMappedView.h"

class MemoryMappedFile
{
public:
	static constexpr UINT64 DefaultFileSize = 64;

	explicit MemoryMappedFile(FileStream& Stream, UINT64 FileSize = DefaultFileSize);
	~MemoryMappedFile();

	MemoryMappedView CreateView();
	MemoryMappedView CreateView(UINT Offset, UINT64 SizeInBytes);

	// Invoking GrowMapping can cause any associated MemoryMappedView to be undefined.
	// Make sure to call CreateView again on any associated views
	void GrowMapping(UINT64 Size);

	UINT64 GetCurrentFileSize() const noexcept { return CurrentFileSize; }

private:
	void InternalCreate(UINT64 FileSize);

private:
	FileStream&			  Stream;
	wil::unique_handle	  FileMapping;
	std::filesystem::path Path;
	UINT64				  CurrentFileSize = 0;
};

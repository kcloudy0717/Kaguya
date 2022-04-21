#pragma once
#include "SystemCore.h"
#include "MemoryMappedView.h"

class FileStream;

class MemoryMappedFile
{
public:
	static constexpr u64 DefaultFileSize = 64;

	explicit MemoryMappedFile(
		FileStream& Stream,
		u64			FileSize = DefaultFileSize);

	MemoryMappedFile(MemoryMappedFile&&) noexcept = default;
	MemoryMappedFile& operator=(MemoryMappedFile&&) noexcept = default;

	MemoryMappedFile(const MemoryMappedFile&) = delete;
	MemoryMappedFile& operator=(const MemoryMappedFile&) = delete;

	[[nodiscard]] u64 GetCurrentFileSize() const noexcept;

	[[nodiscard]] MemoryMappedView CreateView() const noexcept;
	[[nodiscard]] MemoryMappedView CreateView(u32 Offset, u64 SizeInBytes) const noexcept;

	// Invoking GrowMapping can cause any associated MemoryMappedView to be undefined.
	// Make sure to call CreateView again on any associated views
	void GrowMapping(u64 Size);

private:
	void InternalCreate(u64 FileSize);

private:
	FileStream*			  Stream; // Stream must remain valid for the lifetime of the MemoryMappedFile
	wil::unique_handle	  FileMapping;
	std::filesystem::path Path;
	u64					  CurrentFileSize = 0;
};

#pragma once
#include <filesystem>
#include "FileMode.h"
#include "FileAccess.h"
#include "SeekOrigin.h"

class AsyncFileStream
{
public:
	explicit AsyncFileStream(const std::filesystem::path& Path, FileMode Mode);
	explicit AsyncFileStream(const std::filesystem::path& Path, FileMode Mode, FileAccess Access);

	[[nodiscard]] UINT64 GetSizeInBytes() const;

	[[nodiscard]] bool CanRead() const noexcept;
	[[nodiscard]] bool CanWrite() const noexcept;

	AsyncAction Read(void* Buffer, size_t SizeInBytes, uint32_t FilePointerOffset) const;

	AsyncAction Write(const void* Buffer, size_t SizeInBytes, uint32_t FilePointerOffset) const;

private:
	static DWORD GetCreationDisposition(FileMode Mode);
	static DWORD GetDesiredAccess(FileAccess Access);
	static DWORD GetMoveMethod(SeekOrigin Origin);

	void VerifyArguments() const;
	void InternalCreate(const std::filesystem::path& Path, DWORD dwDesiredAccess, DWORD dwCreationDisposition);

	FileMode		   Mode;
	FileAccess		   Access;
	wil::unique_handle Handle;
};

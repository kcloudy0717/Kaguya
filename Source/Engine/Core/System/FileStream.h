#pragma once
#include "FileMode.h"
#include "FileAccess.h"
#include "SeekOrigin.h"

class FileStream
{
public:
	NONCOPYABLE(FileStream);
	DEFAULTMOVABLE(FileStream);

	FileStream() noexcept = default;
	explicit FileStream(const std::filesystem::path& Path, FileMode Mode);
	explicit FileStream(const std::filesystem::path& Path, FileMode Mode, FileAccess Access);

	HANDLE GetHandle() const noexcept { return Handle.get(); }

	void Reset();

	[[nodiscard]] UINT64 GetSizeInBytes() const;

	[[nodiscard]] bool CanRead() const noexcept;
	[[nodiscard]] bool CanWrite() const noexcept;

	[[nodiscard]] std::unique_ptr<BYTE[]> ReadAll() const;

	size_t Read(void* Buffer, size_t SizeInBytes) const;

	void Write(const void* Buffer, size_t SizeInBytes) const;

	void Seek(int64_t Offset, SeekOrigin RelativeOrigin) const;

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

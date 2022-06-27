#pragma once
#include <filesystem>
#include <wil/resource.h>
#include "Types.h"

class FileStream
{
public:
	FileStream() noexcept = default;
	explicit FileStream(
		std::filesystem::path Path,
		FileMode			  Mode,
		FileAccess			  Access);
	explicit FileStream(
		std::filesystem::path Path,
		FileMode			  Mode);

	FileStream(FileStream&&) noexcept = default;
	FileStream& operator=(FileStream&&) noexcept = default;

	FileStream(const FileStream&) = delete;
	FileStream& operator=(const FileStream&) = delete;

	[[nodiscard]] void* GetHandle() const noexcept;
	[[nodiscard]] u64	GetSizeInBytes() const noexcept;
	[[nodiscard]] bool	CanRead() const noexcept;
	[[nodiscard]] bool	CanWrite() const noexcept;

	void Reset();

	[[nodiscard]] std::unique_ptr<std::byte[]> ReadAll() const;

	u64 Read(
		void* Buffer,
		u64	  SizeInBytes) const;

	u64 Write(
		const void* Buffer,
		u64			SizeInBytes) const;

	void Seek(
		i64		   Offset,
		SeekOrigin RelativeOrigin) const;

private:
	std::filesystem::path Path;
	FileMode			  Mode;
	FileAccess			  Access;
	wil::unique_handle	  Handle;
};

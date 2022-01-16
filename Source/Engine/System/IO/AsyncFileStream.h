#pragma once
#include "SystemCore.h"
#include "Async/AsyncAction.h"

class AsyncFileStream
{
public:
	AsyncFileStream() noexcept = default;
	explicit AsyncFileStream(
		const std::filesystem::path& Path,
		FileMode					 Mode,
		FileAccess					 Access);
	explicit AsyncFileStream(
		const std::filesystem::path& Path,
		FileMode					 Mode);

	AsyncFileStream(const AsyncFileStream&) = delete;
	AsyncFileStream& operator=(const AsyncFileStream&) = delete;

	AsyncFileStream(AsyncFileStream&&) noexcept = default;
	AsyncFileStream& operator=(AsyncFileStream&&) noexcept = default;

	[[nodiscard]] void* GetHandle() const noexcept;
	[[nodiscard]] u64	GetSizeInBytes() const noexcept;
	[[nodiscard]] bool	CanRead() const noexcept;
	[[nodiscard]] bool	CanWrite() const noexcept;

	void Reset();

	[[nodiscard]] AsyncAction Read(
		void* Buffer,
		u64	  SizeInBytes,
		u32	  FilePointerOffset) const;

	[[nodiscard]] AsyncAction Write(
		const void* Buffer,
		u64			SizeInBytes,
		u32			FilePointerOffset) const;

private:
	static DWORD GetMoveMethod(SeekOrigin Origin);

	void VerifyArguments();

	wil::unique_handle InitializeHandle(
		const std::filesystem::path& Path,
		FileMode					 Mode,
		FileAccess					 Access);

private:
	std::filesystem::path Path;
	FileMode			  Mode;
	FileAccess			  Access;
	wil::unique_handle	  Handle;
};

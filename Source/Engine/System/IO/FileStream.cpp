#include "FileStream.h"
#include <cassert>

FileStream::FileStream(
	std::filesystem::path Path,
	FileMode			  Mode,
	FileAccess			  Access)
	: Path(std::move(Path))
	, Mode(Mode)
	, Access(Access)
	, Handle(InitializeHandle(this->Path, Mode, Access))
{
}

FileStream::FileStream(
	std::filesystem::path Path,
	FileMode			  Mode)
	: FileStream(std::move(Path), Mode, FileAccess::ReadWrite)
{
}

void* FileStream::GetHandle() const noexcept
{
	return Handle.get();
}

u64 FileStream::GetSizeInBytes() const noexcept
{
	LARGE_INTEGER FileSize = {};
	if (GetFileSizeEx(Handle.get(), &FileSize))
	{
		return FileSize.QuadPart;
	}
	return 0;
}

bool FileStream::CanRead() const noexcept
{
	return Access == FileAccess::Read || Access == FileAccess::ReadWrite;
}

bool FileStream::CanWrite() const noexcept
{
	return Access == FileAccess::Write || Access == FileAccess::ReadWrite;
}

void FileStream::Reset()
{
	Handle.reset();
}

std::unique_ptr<std::byte[]> FileStream::ReadAll() const
{
	u64 FileSize = GetSizeInBytes();

	DWORD NumberOfBytesRead	  = 0;
	DWORD NumberOfBytesToRead = static_cast<DWORD>(FileSize);
	auto  Buffer			  = std::make_unique<std::byte[]>(FileSize);
	if (ReadFile(Handle.get(), Buffer.get(), NumberOfBytesToRead, &NumberOfBytesRead, nullptr))
	{
		assert(NumberOfBytesToRead == NumberOfBytesRead);
	}
	return Buffer;
}

u64 FileStream::Read(
	void* Buffer,
	u64	  SizeInBytes) const
{
	if (Buffer)
	{
		DWORD NumberOfBytesRead	  = 0;
		DWORD NumberOfBytesToRead = static_cast<DWORD>(SizeInBytes);
		if (ReadFile(Handle.get(), Buffer, NumberOfBytesToRead, &NumberOfBytesRead, nullptr))
		{
			return NumberOfBytesRead;
		}
	}
	return 0;
}

u64 FileStream::Write(
	const void* Buffer,
	u64			SizeInBytes) const
{
	if (Buffer)
	{
		DWORD NumberOfBytesWritten = 0;
		DWORD NumberOfBytesToWrite = static_cast<DWORD>(SizeInBytes);
		if (WriteFile(Handle.get(), Buffer, NumberOfBytesToWrite, &NumberOfBytesWritten, nullptr))
		{
			return NumberOfBytesWritten;
		}
	}
	return 0;
}

void FileStream::Seek(
	i64		   Offset,
	SeekOrigin RelativeOrigin) const
{
	DWORD MoveMethod = [RelativeOrigin]()
	{
		// clang-format off
		switch (RelativeOrigin)
		{
		case SeekOrigin::Begin:		return FILE_BEGIN;
		case SeekOrigin::Current:	return FILE_CURRENT;
		case SeekOrigin::End:		return FILE_END;
		}
		// clang-format on
		return 0;
	}();
	LARGE_INTEGER liDistanceToMove = {};
	liDistanceToMove.QuadPart	   = Offset;
	SetFilePointerEx(Handle.get(), liDistanceToMove, nullptr, MoveMethod);
}

wil::unique_handle FileStream::InitializeHandle(
	const std::filesystem::path& Path,
	FileMode					 Mode,
	FileAccess					 Access)
{
	// Validation
	switch (Mode)
	{
	case FileMode::CreateNew:
		assert(CanWrite());
		break;
	case FileMode::Create:
		assert(CanWrite());
		break;
	case FileMode::Open:
		assert(CanRead() || CanWrite());
		break;
	case FileMode::OpenOrCreate:
		assert(CanRead() || CanWrite());
		break;
	case FileMode::Truncate:
		assert(CanWrite());
		break;
	}

	DWORD CreationDisposition = [Mode]
	{
		DWORD dwCreationDisposition = 0;
		// clang-format off
		switch (Mode)
		{
		case FileMode::CreateNew:		dwCreationDisposition = CREATE_NEW;			break;
		case FileMode::Create:			dwCreationDisposition = CREATE_ALWAYS;		break;
		case FileMode::Open:			dwCreationDisposition = OPEN_EXISTING;		break;
		case FileMode::OpenOrCreate:	dwCreationDisposition = OPEN_ALWAYS;		break;
		case FileMode::Truncate:		dwCreationDisposition = TRUNCATE_EXISTING;	break;
		}
		// clang-format on
		return dwCreationDisposition;
	}();

	DWORD DesiredAccess = [Access]
	{
		DWORD dwDesiredAccess = 0;
		// clang-format off
		switch (Access)
		{
		case FileAccess::Read:		dwDesiredAccess = GENERIC_READ;					break;
		case FileAccess::Write:		dwDesiredAccess = GENERIC_WRITE;				break;
		case FileAccess::ReadWrite:	dwDesiredAccess = GENERIC_READ | GENERIC_WRITE; break;
		}
		// clang-format on
		return dwDesiredAccess;
	}();

	auto Handle = wil::unique_handle(CreateFile(
		Path.c_str(),
		DesiredAccess,
		0,
		nullptr,
		CreationDisposition,
		0,
		nullptr));
	if (!Handle)
	{
		DWORD Error = GetLastError();
		if (Mode == FileMode::CreateNew && Error == ERROR_ALREADY_EXISTS)
		{
			throw std::runtime_error("ERROR_ALREADY_EXISTS");
		}
		if (Mode == FileMode::Create && Error == ERROR_ALREADY_EXISTS)
		{
			// File overriden
		}
		if (Mode == FileMode::Open && Error == ERROR_FILE_NOT_FOUND)
		{
			throw std::runtime_error("ERROR_FILE_NOT_FOUND");
		}
		if (Mode == FileMode::OpenOrCreate && Error == ERROR_ALREADY_EXISTS)
		{
			// File exists
		}
	}
	assert(Handle);
	return Handle;
}

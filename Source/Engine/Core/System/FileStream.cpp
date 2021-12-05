#include "FileStream.h"

FileStream::FileStream(const std::filesystem::path& Path, FileMode Mode)
	: Mode(Mode)
	, Access(FileAccess::ReadWrite)
{
	InternalCreate(Path, GetDesiredAccess(FileAccess::ReadWrite), GetCreationDisposition(Mode));
}

FileStream::FileStream(const std::filesystem::path& Path, FileMode Mode, FileAccess Access)
	: Mode(Mode)
	, Access(Access)
{
	InternalCreate(Path, GetDesiredAccess(Access), GetCreationDisposition(Mode));
}

void FileStream::Reset()
{
	Handle.reset();
}

UINT64 FileStream::GetSizeInBytes() const
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

std::unique_ptr<BYTE[]> FileStream::ReadAll() const
{
	UINT64 FileSize				= GetSizeInBytes();
	auto   Buffer				= std::make_unique<BYTE[]>(FileSize);
	DWORD  nNumberOfBytesToRead = static_cast<DWORD>(FileSize);
	DWORD  NumberOfBytesRead	= 0;
	if (ReadFile(Handle.get(), Buffer.get(), nNumberOfBytesToRead, &NumberOfBytesRead, nullptr))
	{
		assert(nNumberOfBytesToRead == NumberOfBytesRead);
	}
	return Buffer;
}

size_t FileStream::Read(void* Buffer, size_t SizeInBytes) const
{
	assert(Buffer);

	BYTE* lpBuffer			   = static_cast<BYTE*>(Buffer);
	DWORD nNumberOfBytesToRead = static_cast<DWORD>(SizeInBytes);
	DWORD NumberOfBytesRead	   = 0;
	if (ReadFile(Handle.get(), lpBuffer, nNumberOfBytesToRead, &NumberOfBytesRead, nullptr))
	{
		return static_cast<size_t>(NumberOfBytesRead);
	}
	return 0;
}

void FileStream::Write(const void* Buffer, size_t SizeInBytes) const
{
	assert(Buffer);

	const BYTE* lpBuffer			  = static_cast<const BYTE*>(Buffer);
	DWORD		nNumberOfBytesToWrite = static_cast<DWORD>(SizeInBytes);
	DWORD		NumberOfBytesWritten  = 0;
	if (!WriteFile(Handle.get(), lpBuffer, nNumberOfBytesToWrite, &NumberOfBytesWritten, nullptr))
	{
		ErrorExit(L"WriteFile");
	}
}

void FileStream::Seek(int64_t Offset, SeekOrigin RelativeOrigin) const
{
	LARGE_INTEGER liDistanceToMove = {};
	liDistanceToMove.QuadPart	   = Offset;
	DWORD dwMoveMethod			   = GetMoveMethod(RelativeOrigin);
	SetFilePointerEx(Handle.get(), liDistanceToMove, nullptr, dwMoveMethod);
}

DWORD FileStream::GetCreationDisposition(FileMode Mode)
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
}

DWORD FileStream::GetDesiredAccess(FileAccess Access)
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
}

DWORD FileStream::GetMoveMethod(SeekOrigin Origin)
{
	DWORD dwMoveMethod = 0;
	// clang-format off
	switch (Origin)
	{
	case SeekOrigin::Begin:		dwMoveMethod = FILE_BEGIN;		break;
	case SeekOrigin::Current:	dwMoveMethod = FILE_CURRENT;	break;
	case SeekOrigin::End:		dwMoveMethod = FILE_END;		break;
	}
	// clang-format on
	return dwMoveMethod;
}

void FileStream::VerifyArguments() const
{
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
}

void FileStream::InternalCreate(const std::filesystem::path& Path, DWORD dwDesiredAccess, DWORD dwCreationDisposition)
{
	VerifyArguments();
	Handle.reset(CreateFile(Path.c_str(), dwDesiredAccess, 0, nullptr, dwCreationDisposition, 0, nullptr));
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
}

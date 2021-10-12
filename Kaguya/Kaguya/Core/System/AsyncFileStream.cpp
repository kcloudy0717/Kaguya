#include "AsyncFileStream.h"

AsyncFileStream::AsyncFileStream(const std::filesystem::path& Path, FileMode Mode)
	: Mode(Mode)
	, Access(FileAccess::ReadWrite)
{
	InternalCreate(Path, GetDesiredAccess(FileAccess::ReadWrite), GetCreationDisposition(Mode));
}

AsyncFileStream::AsyncFileStream(const std::filesystem::path& Path, FileMode Mode, FileAccess Access)
	: Mode(Mode)
	, Access(Access)
{
	InternalCreate(Path, GetDesiredAccess(Access), GetCreationDisposition(Mode));
}

UINT64 AsyncFileStream::GetSizeInBytes() const
{
	LARGE_INTEGER FileSize = {};
	if (GetFileSizeEx(Handle.get(), &FileSize))
	{
		return FileSize.QuadPart;
	}
	return 0;
}

bool AsyncFileStream::CanRead() const noexcept
{
	return Access == FileAccess::Read || Access == FileAccess::ReadWrite;
}

bool AsyncFileStream::CanWrite() const noexcept
{
	return Access == FileAccess::Write || Access == FileAccess::ReadWrite;
}

AsyncAction AsyncFileStream::Read(void* Buffer, size_t SizeInBytes, uint32_t FilePointerOffset) const
{
	assert(Buffer);

	// All locals are stored in the coroutine stack
	wil::unique_event Event;
	Event.create();

	OVERLAPPED Overlapped = {};
	Overlapped.hEvent	  = Event.get();
	Overlapped.Offset	  = FilePointerOffset;

	BYTE* lpBuffer			   = static_cast<BYTE*>(Buffer);
	DWORD nNumberOfBytesToRead = static_cast<DWORD>(SizeInBytes);
	if (!ReadFile(Handle.get(), lpBuffer, nNumberOfBytesToRead, nullptr, &Overlapped))
	{
		if (GetLastError() != ERROR_IO_PENDING)
		{
			ErrorExit(L"ReadFile");
		}
	}

	co_await std::suspend_always{};

	(void)Event.wait();
	assert(HasOverlappedIoCompleted(&Overlapped));
}

AsyncAction AsyncFileStream::Write(const void* Buffer, size_t SizeInBytes, uint32_t FilePointerOffset) const
{
	assert(Buffer);

	// All locals are stored in the coroutine stack
	wil::unique_event Event;
	Event.create();

	OVERLAPPED Overlapped = {};
	Overlapped.hEvent	  = Event.get();
	Overlapped.Offset	  = FilePointerOffset;

	const BYTE* lpBuffer			  = static_cast<const BYTE*>(Buffer);
	DWORD		nNumberOfBytesToWrite = static_cast<DWORD>(SizeInBytes);
	if (!WriteFile(Handle.get(), lpBuffer, nNumberOfBytesToWrite, nullptr, &Overlapped))
	{
		if (GetLastError() != ERROR_IO_PENDING)
		{
			ErrorExit(L"ReadFile");
		}
	}

	co_await std::suspend_always{};

	(void)Event.wait();
	assert(HasOverlappedIoCompleted(&Overlapped));
}

DWORD AsyncFileStream::GetCreationDisposition(FileMode Mode)
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

DWORD AsyncFileStream::GetDesiredAccess(FileAccess Access)
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

DWORD AsyncFileStream::GetMoveMethod(SeekOrigin Origin)
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

void AsyncFileStream::VerifyArguments() const
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

void AsyncFileStream::InternalCreate(
	const std::filesystem::path& Path,
	DWORD						 dwDesiredAccess,
	DWORD						 dwCreationDisposition)
{
	VerifyArguments();
	std::wstring FileName			  = Path.wstring();
	DWORD		 dwFlagsAndAttributes = FILE_FLAG_OVERLAPPED;
	Handle.reset(
		CreateFile(FileName.data(), dwDesiredAccess, 0, nullptr, dwCreationDisposition, dwFlagsAndAttributes, nullptr));
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
}

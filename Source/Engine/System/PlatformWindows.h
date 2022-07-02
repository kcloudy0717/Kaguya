#pragma once
#include <utility>

using WPARAM  = unsigned long long;
using LPARAM  = long long;
using LRESULT = long long;

using HANDLE = void*;

#define WINDOWS_CALLING_CONVENTION __stdcall

#define PLATFORM_WINDOWS_DECLARE_HANDLE(name) \
	struct name##__;                          \
	typedef struct name##__* name

PLATFORM_WINDOWS_DECLARE_HANDLE(HINSTANCE);
PLATFORM_WINDOWS_DECLARE_HANDLE(HWND);

template<typename HandleType>
class ScopedHandle
{
public:
	using NativeHandle = typename HandleType::NativeHandle;

	ScopedHandle()
		: Handle(HandleType::GetInvalidHandle())
	{
	}

	explicit ScopedHandle(NativeHandle Handle)
		: Handle(Handle)
	{
	}

	ScopedHandle(ScopedHandle&& ScopedHandle) noexcept
		: Handle(std::exchange(ScopedHandle.Handle, HandleType::GetInvalidHandle()))
	{
	}

	ScopedHandle& operator=(ScopedHandle&& ScopedHandle) noexcept
	{
		if (this != &ScopedHandle)
		{
			if (HandleType::IsValid(this->Handle))
			{
				HandleType::Destruct(this->Handle);
			}
			this->Handle = std::exchange(ScopedHandle.Handle, HandleType::GetInvalidHandle());
		}
		return *this;
	}

	ScopedHandle& operator=(NativeHandle Handle)
	{
		if (HandleType::IsValid(this->Handle))
		{
			HandleType::Destruct(this->Handle);
		}
		this->Handle = Handle;
		return *this;
	}

	~ScopedHandle()
	{
		if (HandleType::IsValid(Handle))
		{
			HandleType::Destruct(Handle);
		}
	}

	operator bool() const
	{
		return HandleType::IsValid(Handle);
	}

	operator NativeHandle()
	{
		return Handle;
	}

	NativeHandle Get() const noexcept
	{
		return Handle;
	}

	void Reset()
	{
		if (HandleType::IsValid(Handle))
		{
			HandleType::Destruct(Handle);
		}
		Handle = HandleType::GetInvalidHandle();
	}

private:
	NativeHandle Handle;
};

struct CommonHandle
{
	using NativeHandle = HANDLE;

	static NativeHandle GetInvalidHandle();
	static void			Destruct(NativeHandle Handle);
	static bool			IsValid(NativeHandle Handle);
};

struct ProcessHandle : CommonHandle
{
};
struct ThreadHandle : CommonHandle
{
};
struct FileHandle : CommonHandle
{
};
struct EventHandle : CommonHandle
{
};

struct WindowHandle
{
	using NativeHandle = HWND;

	static NativeHandle GetInvalidHandle();
	static void			Destruct(NativeHandle Handle);
	static bool			IsValid(NativeHandle Handle);
};

using ScopedProcessHandle = ScopedHandle<ProcessHandle>;
using ScopedThreadHandle  = ScopedHandle<ThreadHandle>;
using ScopedFileHandle	  = ScopedHandle<FileHandle>;
using ScopedEventHandle	  = ScopedHandle<EventHandle>;
using ScopedWindowHandle  = ScopedHandle<WindowHandle>;

namespace Kaguya
{
	namespace Windows
	{
		struct SRWLOCK
		{
			void* Ptr;
		};

		void InitializeSRWLock(SRWLOCK* SRWLOCK);
		void AcquireSRWLockShared(SRWLOCK* SRWLOCK);
		void ReleaseSRWLockShared(SRWLOCK* SRWLOCK);
		void AcquireSRWLockExclusive(SRWLOCK* SRWLOCK);
		void ReleaseSRWLockExclusive(SRWLOCK* SRWLOCK);

		struct CRITICAL_SECTION
		{
			void* Opaque1[1];
			long  Opaque2[2];
			void* Opaque3[3];
		};

		void InitializeCriticalSectionEx(CRITICAL_SECTION* CRITICAL_SECTION);
		void DeleteCriticalSection(CRITICAL_SECTION* CRITICAL_SECTION);
		void EnterCriticalSection(CRITICAL_SECTION* CRITICAL_SECTION);
		void LeaveCriticalSection(CRITICAL_SECTION* CRITICAL_SECTION);
		bool TryEnterCriticalSection(CRITICAL_SECTION* CRITICAL_SECTION);
	} // namespace Windows
} // namespace Kaguya

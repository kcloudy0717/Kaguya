#include "PlatformWindows.h"
#include <cassert>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <synchapi.h>

namespace Kaguya
{
	namespace Windows
	{
		void InitializeSRWLock(SRWLOCK* SRWLOCK)
		{
			::InitializeSRWLock(reinterpret_cast<PSRWLOCK>(SRWLOCK));
		}

		void AcquireSRWLockShared(SRWLOCK* SRWLOCK)
		{
			::AcquireSRWLockShared(reinterpret_cast<PSRWLOCK>(SRWLOCK));
		}

		void ReleaseSRWLockShared(SRWLOCK* SRWLOCK)
		{
			::ReleaseSRWLockShared(reinterpret_cast<PSRWLOCK>(SRWLOCK));
		}

		void AcquireSRWLockExclusive(SRWLOCK* SRWLOCK)
		{
			::AcquireSRWLockExclusive(reinterpret_cast<PSRWLOCK>(SRWLOCK));
		}

		void ReleaseSRWLockExclusive(SRWLOCK* SRWLOCK)
		{
			::ReleaseSRWLockExclusive(reinterpret_cast<PSRWLOCK>(SRWLOCK));
		}

		void InitializeCriticalSectionEx(CRITICAL_SECTION* CRITICAL_SECTION)
		{
			BOOL Result = ::InitializeCriticalSectionEx(reinterpret_cast<PCRITICAL_SECTION>(CRITICAL_SECTION), 0, 0);
			assert(Result);
		}

		void DeleteCriticalSection(CRITICAL_SECTION* CRITICAL_SECTION)
		{
			::DeleteCriticalSection(reinterpret_cast<PCRITICAL_SECTION>(CRITICAL_SECTION));
		}

		void EnterCriticalSection(CRITICAL_SECTION* CRITICAL_SECTION)
		{
			::EnterCriticalSection(reinterpret_cast<PCRITICAL_SECTION>(CRITICAL_SECTION));
		}

		void LeaveCriticalSection(CRITICAL_SECTION* CRITICAL_SECTION)
		{
			::LeaveCriticalSection(reinterpret_cast<PCRITICAL_SECTION>(CRITICAL_SECTION));
		}

		bool TryEnterCriticalSection(CRITICAL_SECTION* CRITICAL_SECTION)
		{
			return ::TryEnterCriticalSection(reinterpret_cast<PCRITICAL_SECTION>(CRITICAL_SECTION));
		}
	} // namespace Windows
} // namespace Kaguya

CommonHandle::NativeHandle CommonHandle::GetInvalidHandle()
{
	return INVALID_HANDLE_VALUE;
}

void CommonHandle::Destruct(NativeHandle Handle)
{
	::CloseHandle(Handle);
}

bool CommonHandle::IsValid(NativeHandle Handle)
{
	return Handle != GetInvalidHandle();
}

ModuleHandle::NativeHandle ModuleHandle::GetInvalidHandle()
{
	return nullptr;
}

void ModuleHandle::Destruct(NativeHandle Handle)
{
	::FreeLibrary(Handle);
}

bool ModuleHandle::IsValid(NativeHandle Handle)
{
	return Handle != GetInvalidHandle();
}

WindowHandle::NativeHandle WindowHandle::GetInvalidHandle()
{
	return nullptr;
}

void WindowHandle::Destruct(NativeHandle Handle)
{
	::DestroyWindow(Handle);
}

bool WindowHandle::IsValid(NativeHandle Handle)
{
	return Handle != GetInvalidHandle();
}

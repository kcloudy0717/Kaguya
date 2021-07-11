#pragma once
#include <synchapi.h>

// BasicLockable requirement
// Lockable requirement
class CriticalSection
{
public:
	CriticalSection(DWORD SpinCount = 0, DWORD Flags = 0) { ::InitializeCriticalSectionEx(&Handle, SpinCount, Flags); }
	~CriticalSection() { ::DeleteCriticalSection(&Handle); }

	CriticalSection(const CriticalSection&) = delete;
	CriticalSection& operator=(const CriticalSection&) = delete;

	void lock() { ::EnterCriticalSection(&Handle); }

	void unlock() noexcept { ::LeaveCriticalSection(&Handle); }

	bool try_lock() { return ::TryEnterCriticalSection(&Handle); }

private:
	CRITICAL_SECTION Handle;
};

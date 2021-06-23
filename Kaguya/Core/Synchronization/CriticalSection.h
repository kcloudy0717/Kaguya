#pragma once
#include <synchapi.h>

// BasicLockable requirement
// Lockable requirement
class CriticalSection
{
public:
	CriticalSection(DWORD SpinCount = 0, DWORD Flags = 0)
	{
		::InitializeCriticalSectionEx(&m_CriticalSection, SpinCount, Flags);
	}

	~CriticalSection() { ::DeleteCriticalSection(&m_CriticalSection); }

	void lock() { ::EnterCriticalSection(&m_CriticalSection); }

	void unlock() noexcept { ::LeaveCriticalSection(&m_CriticalSection); }

	bool try_lock() { return ::TryEnterCriticalSection(&m_CriticalSection); }

	operator auto() { return &m_CriticalSection; }

	operator auto &() { return m_CriticalSection; }

	operator auto &() const { return m_CriticalSection; }

private:
	CRITICAL_SECTION m_CriticalSection;
};

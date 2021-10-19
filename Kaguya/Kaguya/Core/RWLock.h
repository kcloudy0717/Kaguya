#pragma once
#include <synchapi.h>

struct STATIC_RWLOCK
{
};

class RWLock
{
public:
	RWLock() { ::InitializeSRWLock(&Handle); }
	RWLock(STATIC_RWLOCK)
		: Handle(SRWLOCK_INIT)
	{
	}
	~RWLock() {}

	RWLock(const RWLock&) = delete;
	RWLock& operator=(const RWLock&) = delete;

	void AcquireShared() { ::AcquireSRWLockShared(&Handle); }

	void ReleaseShared() { ::ReleaseSRWLockShared(&Handle); }

	void AcquireExclusive() { ::AcquireSRWLockExclusive(&Handle); }

	void ReleaseExclusive() { ::ReleaseSRWLockExclusive(&Handle); }

private:
	SRWLOCK Handle;
};

class ScopedReadLock
{
public:
	ScopedReadLock(RWLock& Lock)
		: Lock(Lock)
	{
		Lock.AcquireShared();
	}
	~ScopedReadLock() { Lock.ReleaseShared(); }

private:
	RWLock& Lock;
};

class ScopedWriteLock
{
public:
	ScopedWriteLock(RWLock& Lock)
		: Lock(Lock)
	{
		Lock.AcquireExclusive();
	}
	~ScopedWriteLock() { Lock.ReleaseExclusive(); }

private:
	RWLock& Lock;
};

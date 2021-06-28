#pragma once
#include <synchapi.h>

class RWLock
{
public:
	RWLock() { ::InitializeSRWLock(&Handle); }
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

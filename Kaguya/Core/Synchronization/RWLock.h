#pragma once
#include <synchapi.h>

class RWLock : public SRWLOCK
{
public:
	RWLock() { ::InitializeSRWLock(this); }

	~RWLock() {}

	void AcquireShared() { ::AcquireSRWLockShared(this); }

	void ReleaseShared() { ::ReleaseSRWLockShared(this); }

	void AcquireExclusive() { ::AcquireSRWLockExclusive(this); }

	void ReleaseExclusive() { ::ReleaseSRWLockExclusive(this); }
};

class ScopedReadLock
{
public:
	ScopedReadLock(SRWLOCK& Lock)
		: Lock(Lock)
	{
		::AcquireSRWLockShared(&Lock);
	}

	~ScopedReadLock() { ::ReleaseSRWLockShared(&Lock); }

private:
	SRWLOCK& Lock;
};

class ScopedWriteLock
{
public:
	ScopedWriteLock(SRWLOCK& Lock)
		: Lock(Lock)
	{
		::AcquireSRWLockExclusive(&Lock);
	}

	~ScopedWriteLock() { ::ReleaseSRWLockExclusive(&Lock); }

private:
	SRWLOCK& Lock;
};

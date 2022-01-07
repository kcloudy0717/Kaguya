#pragma once

class Mutex;
class RwLock;

class MutexGuard
{
public:
	explicit MutexGuard(Mutex& Ref);
	~MutexGuard();

private:
	Mutex& Ref;
};

class Mutex
{
public:
	Mutex();
	~Mutex();

	Mutex(const Mutex&)		= delete;
	Mutex(Mutex&&) noexcept = delete;
	Mutex& operator=(const Mutex&) = delete;
	Mutex& operator=(Mutex&&) noexcept = delete;

	void Enter();

	void Leave();

	bool TryEnter();

private:
	CRITICAL_SECTION Handle;
};

class RwLockReadGuard
{
public:
	explicit RwLockReadGuard(RwLock& Ref);
	~RwLockReadGuard();

private:
	RwLock& Ref;
};

class RwLockWriteGuard
{
public:
	explicit RwLockWriteGuard(RwLock& Ref);
	~RwLockWriteGuard();

private:
	RwLock& Ref;
};

class RwLock
{
public:
	RwLock();

	RwLock(const RwLock&)	  = delete;
	RwLock(RwLock&&) noexcept = delete;
	RwLock& operator=(const RwLock&) = delete;
	RwLock& operator=(RwLock&&) noexcept = delete;

	void AcquireShared();

	void ReleaseShared();

	void AcquireExclusive();

	void ReleaseExclusive();

private:
	SRWLOCK Handle;
};

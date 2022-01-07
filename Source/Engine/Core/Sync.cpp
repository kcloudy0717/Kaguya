#include "Sync.h"

MutexGuard::MutexGuard(Mutex& Ref)
	: Ref(Ref)
{
	Ref.Enter();
}

MutexGuard::~MutexGuard()
{
	Ref.Leave();
}

Mutex::Mutex()
{
	InitializeCriticalSectionEx(&Handle, 0, 0);
}

Mutex::~Mutex()
{
	DeleteCriticalSection(&Handle);
}

void Mutex::Enter()
{
	EnterCriticalSection(&Handle);
}

void Mutex::Leave()
{
	LeaveCriticalSection(&Handle);
}

bool Mutex::TryEnter()
{
	return TryEnterCriticalSection(&Handle);
}

RwLockReadGuard::RwLockReadGuard(RwLock& Ref)
	: Ref(Ref)
{
	Ref.AcquireShared();
}

RwLockReadGuard::~RwLockReadGuard()
{
	Ref.ReleaseShared();
}

RwLockWriteGuard::RwLockWriteGuard(RwLock& Ref)

	: Ref(Ref)
{
	Ref.AcquireExclusive();
}

RwLockWriteGuard::~RwLockWriteGuard()
{
	Ref.ReleaseExclusive();
}

RwLock::RwLock()
{
	InitializeSRWLock(&Handle);
}

void RwLock::AcquireShared()
{
	AcquireSRWLockShared(&Handle);
}

void RwLock::ReleaseShared()
{
	ReleaseSRWLockShared(&Handle);
}

void RwLock::AcquireExclusive()
{
	AcquireSRWLockExclusive(&Handle);
}

void RwLock::ReleaseExclusive()
{
	ReleaseSRWLockExclusive(&Handle);
}

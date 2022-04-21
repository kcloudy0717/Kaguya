#include "RwLock.h"

RwLock::RwLock()
{
	InitializeSRWLock(&Handle);
}

RwLock::~RwLock()
{
	UNREFERENCED_PARAMETER(Handle);
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

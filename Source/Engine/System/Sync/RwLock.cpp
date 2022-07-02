#include "RwLock.h"

RwLock::RwLock()
{
	Kaguya::Windows::InitializeSRWLock(&Handle);
}

RwLock::~RwLock()
{
}

void RwLock::AcquireShared()
{
	Kaguya::Windows::AcquireSRWLockShared(&Handle);
}

void RwLock::ReleaseShared()
{
	Kaguya::Windows::ReleaseSRWLockShared(&Handle);
}

void RwLock::AcquireExclusive()
{
	Kaguya::Windows::AcquireSRWLockExclusive(&Handle);
}

void RwLock::ReleaseExclusive()
{
	Kaguya::Windows::ReleaseSRWLockExclusive(&Handle);
}

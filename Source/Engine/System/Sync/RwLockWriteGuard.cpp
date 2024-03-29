#include "RwLockWriteGuard.h"
#include <utility>
#include "RwLock.h"

RwLockWriteGuard::RwLockWriteGuard(RwLock& Ref)
	: Ptr(&Ref)
{
	Ptr->AcquireExclusive();
}

RwLockWriteGuard::RwLockWriteGuard(RwLockWriteGuard&& RwLockWriteGuard) noexcept
	: Ptr(std::exchange(RwLockWriteGuard.Ptr, nullptr))
{
}

RwLockWriteGuard& RwLockWriteGuard::operator=(RwLockWriteGuard&& RwLockWriteGuard) noexcept
{
	if (this != &RwLockWriteGuard)
	{
		Ptr = std::exchange(RwLockWriteGuard.Ptr, nullptr);
	}
	return *this;
}

RwLockWriteGuard::~RwLockWriteGuard()
{
	if (Ptr)
	{
		Ptr->ReleaseExclusive();
	}
}

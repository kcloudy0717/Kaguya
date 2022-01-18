#include "RwLockReadGuard.h"
#include "RwLock.h"

RwLockReadGuard::RwLockReadGuard(RwLock& Ref)
	: Ptr(&Ref)
{
	Ptr->AcquireShared();
}

RwLockReadGuard::RwLockReadGuard(RwLockReadGuard&& RwLockReadGuard) noexcept
	: Ptr(std::exchange(RwLockReadGuard.Ptr, nullptr))
{
}

RwLockReadGuard& RwLockReadGuard::operator=(RwLockReadGuard&& RwLockReadGuard) noexcept
{
	if (this == &RwLockReadGuard)
	{
		return *this;
	}

	Ptr = std::exchange(RwLockReadGuard.Ptr, nullptr);
	return *this;
}

RwLockReadGuard::~RwLockReadGuard()
{
	if (Ptr)
	{
		Ptr->ReleaseShared();
	}
}

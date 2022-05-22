#include "MutexGuard.h"
#include <utility>
#include "Mutex.h"

MutexGuard::MutexGuard(Mutex& Ref)
	: Ptr(&Ref)
{
	Ptr->Enter();
}

MutexGuard::MutexGuard(MutexGuard&& MutexGuard) noexcept
	: Ptr(std::exchange(MutexGuard.Ptr, nullptr))
{
}

MutexGuard& MutexGuard::operator=(MutexGuard&& MutexGuard) noexcept
{
	if (this != &MutexGuard)
	{
		Ptr = std::exchange(MutexGuard.Ptr, nullptr);
	}
	return *this;
}

MutexGuard::~MutexGuard()
{
	if (Ptr)
	{
		Ptr->Leave();
	}
}

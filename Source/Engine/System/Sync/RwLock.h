#pragma once
#include "SystemCore.h"

class RwLock
{
public:
	RwLock();
	~RwLock();

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

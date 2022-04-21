#pragma once
#include "SystemCore.h"

class RwLock;

class RwLockWriteGuard
{
public:
	RwLockWriteGuard() noexcept = default;
	explicit RwLockWriteGuard(RwLock& Ref);
	RwLockWriteGuard(RwLockWriteGuard&& RwLockWriteGuard) noexcept;
	RwLockWriteGuard& operator=(RwLockWriteGuard&& RwLockWriteGuard) noexcept;
	~RwLockWriteGuard();

	RwLockWriteGuard(const RwLockWriteGuard&) = delete;
	RwLockWriteGuard& operator=(const RwLockWriteGuard&) = delete;

private:
	RwLock* Ptr = nullptr;
};

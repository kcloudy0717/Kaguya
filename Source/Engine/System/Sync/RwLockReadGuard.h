#pragma once
#include "SystemCore.h"

class RwLock;

class RwLockReadGuard
{
public:
	RwLockReadGuard() noexcept = default;
	explicit RwLockReadGuard(RwLock& Ref);
	RwLockReadGuard(RwLockReadGuard&& RwLockReadGuard) noexcept;
	RwLockReadGuard& operator=(RwLockReadGuard&& RwLockReadGuard) noexcept;
	~RwLockReadGuard();

	RwLockReadGuard(const RwLockReadGuard&) = delete;
	RwLockReadGuard& operator=(const RwLockReadGuard&) = delete;

private:
	RwLock* Ptr = nullptr;
};

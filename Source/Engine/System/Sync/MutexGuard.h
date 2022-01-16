#pragma once

class Mutex;

class MutexGuard
{
public:
	MutexGuard() noexcept = default;
	explicit MutexGuard(Mutex& Ref);
	MutexGuard(MutexGuard&& MutexGuard) noexcept;
	MutexGuard& operator=(MutexGuard&& MutexGuard) noexcept;
	~MutexGuard();

	MutexGuard(const MutexGuard&) = delete;
	MutexGuard& operator=(const MutexGuard&) = delete;

private:
	Mutex* Ptr = nullptr;
};

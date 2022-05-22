#pragma once
#include "Delegate.h"

class ThreadPool
{
public:
	using ThreadpoolWork = Delegate<void(void* /*Context*/)>;

	static std::unique_ptr<ThreadPool> Create();

	ThreadPool() noexcept = default;
	virtual ~ThreadPool() = default;

	// Noncopyable, non-movable since Environment is not a pointer.
	ThreadPool(const ThreadPool&) = delete;
	ThreadPool(ThreadPool&&)	  = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;
	ThreadPool& operator=(ThreadPool&&) = delete;

	virtual void QueueThreadpoolWork(ThreadpoolWork&& Callback, void* Context) = 0;
};

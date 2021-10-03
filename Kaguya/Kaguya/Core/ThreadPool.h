#pragma once
#include <threadpoolapiset.h>
#include "Delegate.h"

class ThreadPoolWork
{
public:
	friend class ThreadPool;

	ThreadPoolWork() noexcept = default;

	ThreadPoolWork(const ThreadPoolWork&)	  = delete;
	ThreadPoolWork(ThreadPoolWork&&) noexcept = delete;
	ThreadPoolWork& operator=(const ThreadPoolWork&) = delete;
	ThreadPoolWork& operator=(ThreadPoolWork&&) noexcept = delete;

	void Wait(bool Cancel = true)
	{
		if (Work)
		{
			WaitForThreadpoolWorkCallbacks(Work, Cancel);
			CloseThreadpoolWork(Work);
			Work = nullptr;
		}
	}
	operator bool() const { return Work != nullptr; }

	~ThreadPoolWork() { Wait(true); }

private:
	static void CALLBACK WorkCallback(PTP_CALLBACK_INSTANCE, PVOID Context, PTP_WORK)
	{
		auto Work = static_cast<ThreadPoolWork*>(Context);
		Work->WorkDelegate();
	}

private:
	PTP_WORK		 Work = nullptr;
	std::function<void()> WorkDelegate;
};

class ThreadPool
{
public:
	ThreadPool();
	~ThreadPool();

	// Noncopyable, non-movable since Environment is not a pointer.
	ThreadPool(const ThreadPool&) = delete;
	ThreadPool(ThreadPool&&)	  = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;
	ThreadPool& operator=(ThreadPool&&) = delete;

	void SetCancelPendingWorkOnCleanup(bool bCancel) { bCancelPendingWorkOnCleanup = bCancel; }

	void QueueThreadpoolWork(ThreadPoolWork* Work, std::function<void()> WorkFunction);

private:
	TP_CALLBACK_ENVIRON Environment;
	PTP_POOL			Pool						= nullptr;
	PTP_CLEANUP_GROUP	CleanupGroup				= nullptr;
	bool				bCancelPendingWorkOnCleanup = true;
};

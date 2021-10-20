#pragma once
#include <threadpoolapiset.h>

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

	void QueueThreadpoolWork(PTP_WORK_CALLBACK Callback, PVOID Context);

private:
	TP_CALLBACK_ENVIRON Environment;
	PTP_POOL			Pool						= nullptr;
	PTP_CLEANUP_GROUP	CleanupGroup				= nullptr;
	bool				bCancelPendingWorkOnCleanup = true;
};

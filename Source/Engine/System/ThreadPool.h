#pragma once
#include "SystemCore.h"
#include "Delegate.h"
#include <threadpoolapiset.h>

class ThreadPool
{
public:
	using ThreadpoolWork = Delegate<void(void* /*Context*/)>;

	ThreadPool();
	~ThreadPool();

	// Noncopyable, non-movable since Environment is not a pointer.
	ThreadPool(const ThreadPool&) = delete;
	ThreadPool(ThreadPool&&)	  = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;
	ThreadPool& operator=(ThreadPool&&) = delete;

	void SetCancelPendingWorkOnCleanup(bool bCancel) { bCancelPendingWorkOnCleanup = bCancel; }

	void QueueThreadpoolWork(ThreadpoolWork&& Callback, PVOID Context);

private:
	static VOID NTAPI ThreadpoolWorkCallback(
		_Inout_ PTP_CALLBACK_INSTANCE Instance,
		_Inout_opt_ PVOID			  Context,
		_Inout_ PTP_WORK			  Work) noexcept;

private:
	TP_CALLBACK_ENVIRON Environment;
	PTP_POOL			Pool						= nullptr;
	PTP_CLEANUP_GROUP	CleanupGroup				= nullptr;
	bool				bCancelPendingWorkOnCleanup = true;

	struct WorkEntry
	{
		ThreadpoolWork Work;
		void*		   Context = nullptr;
	};
};

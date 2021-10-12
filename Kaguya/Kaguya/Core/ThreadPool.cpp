#include "ThreadPool.h"

ThreadPool::ThreadPool()
{
	InitializeThreadpoolEnvironment(&Environment);
	Pool = CreateThreadpool(nullptr);
	if (!Pool)
	{
		throw std::exception("CreateThreadpool failed");
	}

	CleanupGroup = CreateThreadpoolCleanupGroup();
	if (!CleanupGroup)
	{
		CloseThreadpool(Pool);
		throw std::exception("CreateThreadpoolCleanupGroup failed");
	}

	// No more failures
	SetThreadpoolCallbackPool(&Environment, Pool);
	SetThreadpoolCallbackCleanupGroup(&Environment, CleanupGroup, nullptr);
}

ThreadPool::~ThreadPool()
{
	CloseThreadpoolCleanupGroupMembers(CleanupGroup, bCancelPendingWorkOnCleanup, nullptr);
	CloseThreadpoolCleanupGroup(CleanupGroup);
	CloseThreadpool(Pool);
	DestroyThreadpoolEnvironment(&Environment);
}

void ThreadPool::QueueThreadpoolWork(PTP_WORK_CALLBACK Callback, PVOID Context)
{
	assert(Callback);

	PTP_WORK Work = CreateThreadpoolWork(Callback, Context, &Environment);
	if (!Work)
	{
		throw std::exception("CreateThreadpoolWork failed");
	}
	SubmitThreadpoolWork(Work);
}

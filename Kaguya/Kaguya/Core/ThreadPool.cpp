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

void ThreadPool::QueueThreadpoolWork(ThreadPoolWork* Work, std::function<void()> WorkFunction)
{
	assert(!Work->Work);

	Work->WorkDelegate = std::move(WorkFunction);
	Work->Work		   = CreateThreadpoolWork(&ThreadPoolWork::WorkCallback, Work, &Environment);
	if (!Work->Work)
	{
		throw std::exception("CreateThreadpoolWork failed");
	}

	SubmitThreadpoolWork(Work->Work);
}

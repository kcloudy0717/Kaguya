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

void ThreadPool::QueueThreadpoolWork(ThreadpoolWork&& Callback, PVOID Context)
{
	assert(Callback);

	std::unique_ptr<WorkEntry> Entry(new (std::nothrow) WorkEntry());
	if (Entry)
	{
		Entry->Work	   = std::move(Callback);
		Entry->Context = Context;

		PTP_WORK Work = CreateThreadpoolWork(ThreadpoolWorkCallback, Entry.release(), &Environment);
		if (!Work)
		{
			throw std::exception("CreateThreadpoolWork failed");
		}
		SubmitThreadpoolWork(Work);
	}
}

VOID NTAPI ThreadPool::ThreadpoolWorkCallback(
	_Inout_ PTP_CALLBACK_INSTANCE Instance,
	_Inout_opt_ PVOID			  Context,
	_Inout_ PTP_WORK			  Work) noexcept
{
	UNREFERENCED_PARAMETER(Instance);
	auto Entry = static_cast<WorkEntry*>(Context);
	Entry->Work(Entry->Context);
	delete Entry;
	CloseThreadpoolWork(Work);
}

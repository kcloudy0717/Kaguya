#include "ThreadPool.h"
#include <cassert>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

struct WorkEntry
{
	ThreadPool::ThreadpoolWork Work;
	void*					   Context = nullptr;
};

namespace OS
{
	namespace Internal
	{
		static VOID NTAPI ThreadpoolWorkCallback(
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
	} // namespace Internal
} // namespace OS

class WindowsThreadPool : public ThreadPool
{
public:
	WindowsThreadPool()
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
	~WindowsThreadPool()
	{
		CloseThreadpoolCleanupGroupMembers(CleanupGroup, bCancelPendingWorkOnCleanup, nullptr);
		CloseThreadpoolCleanupGroup(CleanupGroup);
		CloseThreadpool(Pool);
		DestroyThreadpoolEnvironment(&Environment);
	}

	void QueueThreadpoolWork(ThreadpoolWork&& Callback, void* Context) override
	{
		assert(Callback);

		std::unique_ptr<WorkEntry> Entry(new (std::nothrow) WorkEntry());
		if (Entry)
		{
			Entry->Work	   = std::move(Callback);
			Entry->Context = Context;

			PTP_WORK Work = CreateThreadpoolWork(OS::Internal::ThreadpoolWorkCallback, Entry.release(), &Environment);
			if (!Work)
			{
				throw std::runtime_error("CreateThreadpoolWork");
			}
			SubmitThreadpoolWork(Work);
		}
	}

private:
	TP_CALLBACK_ENVIRON Environment;
	PTP_POOL			Pool						= nullptr;
	PTP_CLEANUP_GROUP	CleanupGroup				= nullptr;
	bool				bCancelPendingWorkOnCleanup = true;
};

std::unique_ptr<ThreadPool> ThreadPool::Create()
{
	return std::make_unique<WindowsThreadPool>();
}

#include "Process.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>

struct WorkEntry
{
	ThreadPool::ThreadPoolWork Work;
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
			if (Context)
			{
				auto Entry = static_cast<WorkEntry*>(Context);
				Entry->Work(Entry->Context);
				delete Entry;
			}
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
	~WindowsThreadPool() override
	{
		CloseThreadpoolCleanupGroupMembers(CleanupGroup, TRUE, nullptr);
		CloseThreadpoolCleanupGroup(CleanupGroup);
		CloseThreadpool(Pool);
		DestroyThreadpoolEnvironment(&Environment);
	}

	void QueueThreadpoolWork(ThreadPoolWork&& Callback, void* Context) override
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
	PTP_POOL			Pool		 = nullptr;
	PTP_CLEANUP_GROUP	CleanupGroup = nullptr;
};

const std::filesystem::path Process::ExecutableDirectory = []
{
	std::filesystem::path Path;
	int					  Argc = 0;
	if (LPWSTR* Argv = CommandLineToArgvW(GetCommandLineW(), &Argc); Argv)
	{
		Path = std::filesystem::path(Argv[0]).parent_path();
		LocalFree(Argv);
	}

	return Path;
}();

ThreadPool& Process::GetThreadPool()
{
	static WindowsThreadPool ThreadPool;
	return ThreadPool;
}

ThreadId Process::GetCurrentThreadId()
{
	return { ::GetCurrentThreadId() };
}

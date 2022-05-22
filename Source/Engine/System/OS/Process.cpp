#include "Process.h"
#define WIN32_LEAN_AND_MEAN
#include <Window.h>
#include <shellapi.h>

const std::filesystem::path Process::ExecutableDirectory = []
{
	std::filesystem::path Path;
	int					  Argc;
	if (LPWSTR* Argv = CommandLineToArgvW(GetCommandLineW(), &Argc); Argv)
	{
		Path = std::filesystem::path(Argv[0]).parent_path();
		LocalFree(Argv);
	}

	return Path;
}();

std::unique_ptr<ThreadPool> Process::ThreadPool = ThreadPool::Create();

#include "Process.h"
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

ThreadPool Process::ThreadPool;

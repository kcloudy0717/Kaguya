#pragma once
#include <filesystem>
#include "ThreadPool.h"

struct ThreadId
{
	u32 Id;
};

class Process
{
public:
	static const std::filesystem::path ExecutableDirectory;

	static ThreadPool& GetThreadPool();

	static ThreadId GetCurrentThreadId();
};

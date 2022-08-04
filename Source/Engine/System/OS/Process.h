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

	static std::unique_ptr<ThreadPool> ThreadPool;

	static ThreadId GetCurrentThreadId();
};

#pragma once
#include "SystemCore.h"
#include "ThreadPool.h"

class Process
{
public:
	static const std::filesystem::path ExecutableDirectory;

	static ThreadPool ThreadPool;
};

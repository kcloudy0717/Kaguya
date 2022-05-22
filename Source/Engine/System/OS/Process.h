#pragma once
#include <filesystem>
#include "ThreadPool.h"

class Process
{
public:
	static const std::filesystem::path ExecutableDirectory;

	static std::unique_ptr<ThreadPool> ThreadPool;
};

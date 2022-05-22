#pragma once
#include <filesystem>

class Directory
{
public:
	static bool Exists(const std::filesystem::path& Path);
};

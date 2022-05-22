#pragma once
#include <filesystem>

class File
{
public:
	static bool Exists(const std::filesystem::path& Path);
};

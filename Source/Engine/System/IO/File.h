#pragma once
#include <filesystem>
#include "FileStream.h"

class File
{
public:
	[[nodiscard]] static bool Exists(const std::filesystem::path& Path);

	// Creates or overwrites a file in the specified path.
	[[nodiscard]] static FileStream Create(const std::filesystem::path& Path);
};

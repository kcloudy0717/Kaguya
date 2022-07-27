#pragma once
#include <filesystem>
#include "FileStream.h"

class File
{
public:
	[[nodiscard]] static bool Exists(const std::filesystem::path& Path);

	// Creates or overwrites a file in the specified path.
	[[nodiscard]] static FileStream Create(const std::filesystem::path& Path);
	// Read a file from the specified path.
	// throws if the file does not exist.
	[[nodiscard]] static FileStream Read(const std::filesystem::path& Path);
};

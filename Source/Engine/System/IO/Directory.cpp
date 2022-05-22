#include "Directory.h"
#define WIN32_LEAN_AND_MEAN
#include <Window.h>

bool Directory::Exists(const std::filesystem::path& Path)
{
	const DWORD FileAttributes = GetFileAttributes(Path.c_str());
	return FileAttributes != INVALID_FILE_ATTRIBUTES && (FileAttributes & FILE_ATTRIBUTE_DIRECTORY);
}

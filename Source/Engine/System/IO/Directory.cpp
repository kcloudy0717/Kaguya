#include "Directory.h"

bool Directory::Exists(const std::filesystem::path& Path)
{
	DWORD FileAttributes = GetFileAttributes(Path.c_str());
	return (FileAttributes != INVALID_FILE_ATTRIBUTES && (FileAttributes & FILE_ATTRIBUTE_DIRECTORY));
}

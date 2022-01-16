#pragma once
#include "SystemCore.h"

class File
{
public:
	static bool Exists(const std::filesystem::path& Path);
};

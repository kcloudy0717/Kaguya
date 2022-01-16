#pragma once
#include "SystemCore.h"

class Directory
{
public:
	static bool Exists(const std::filesystem::path& Path);
};

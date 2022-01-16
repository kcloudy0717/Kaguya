#pragma once
#include <span>
#include "SystemCore.h"

struct FilterDesc
{
	LPCWSTR Name;
	LPCWSTR Filter;
};

class FileSystem
{
public:
	static std::filesystem::path OpenDialog(std::span<FilterDesc> FilterSpecs);
	static std::filesystem::path SaveDialog(std::span<FilterDesc> FilterSpecs);
};

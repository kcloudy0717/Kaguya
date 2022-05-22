#pragma once
#include <filesystem>
#include "Span.h"

struct FilterDesc
{
	std::wstring_view Name;
	std::wstring_view Filter;
};

class FileSystem
{
public:
	static std::filesystem::path			  OpenDialog(Span<const FilterDesc> FilterSpecs);
	static std::vector<std::filesystem::path> OpenDialogMultiple(Span<const FilterDesc> FilterSpecs);
	static std::filesystem::path			  SaveDialog(Span<const FilterDesc> FilterSpecs);
};

#pragma once

class FileSystem
{
public:
	static std::filesystem::path OpenDialog(std::span<COMDLG_FILTERSPEC> FilterSpecs);
	static std::filesystem::path SaveDialog(std::span<COMDLG_FILTERSPEC> FilterSpecs);
};

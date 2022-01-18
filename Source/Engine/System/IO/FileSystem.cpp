﻿#include "FileSystem.h"
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

std::filesystem::path FileSystem::OpenDialog(std::span<FilterDesc> FilterSpecs)
{
	// COMDLG_FILTERSPEC ComDlgFS[3] = { { L"C++ code files", L"*.cpp;*.h;*.rc" },
	//								  { L"Executable Files", L"*.exe;*.dll" },
	//								  { L"All Files (*.*)", L"*.*" } };
	std::vector<COMDLG_FILTERSPEC> Specs(FilterSpecs.size());
	for (size_t i = 0; i < Specs.size(); ++i)
	{
		Specs[i].pszName = FilterSpecs[i].Name;
		Specs[i].pszSpec = FilterSpecs[i].Filter;
	}

	std::filesystem::path	Path;
	ComPtr<IFileOpenDialog> FileOpen;
	if (SUCCEEDED(CoCreateInstance(
			CLSID_FileOpenDialog,
			nullptr,
			CLSCTX_ALL,
			IID_PPV_ARGS(&FileOpen))))
	{
		FileOpen->SetFileTypes(static_cast<UINT>(Specs.size()), Specs.data());

		// Show the Open dialog box.
		// Get the file name from the dialog box.
		if (SUCCEEDED(FileOpen->Show(nullptr)))
		{
			ComPtr<IShellItem> Item;
			if (SUCCEEDED(FileOpen->GetResult(&Item)))
			{
				PWSTR pszFilePath;
				// Display the file name to the user.
				if (SUCCEEDED(Item->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath)))
				{
					Path = pszFilePath;
					CoTaskMemFree(pszFilePath);
				}
			}
		}
	}

	return Path;
}

std::filesystem::path FileSystem::SaveDialog(std::span<FilterDesc> FilterSpecs)
{
	std::vector<COMDLG_FILTERSPEC> Specs(FilterSpecs.size());
	for (size_t i = 0; i < Specs.size(); ++i)
	{
		Specs[i].pszName = FilterSpecs[i].Name;
		Specs[i].pszSpec = FilterSpecs[i].Filter;
	}

	std::filesystem::path	Path;
	ComPtr<IFileSaveDialog> FileSave;
	if (SUCCEEDED(CoCreateInstance(
			CLSID_FileSaveDialog,
			nullptr,
			CLSCTX_ALL,
			IID_PPV_ARGS(&FileSave))))
	{
		FileSave->SetFileTypes(static_cast<UINT>(Specs.size()), Specs.data());

		// Show the Save dialog box.
		// Get the file name from the dialog box.
		if (SUCCEEDED(FileSave->Show(nullptr)))
		{
			ComPtr<IShellItem> Item;
			if (SUCCEEDED(FileSave->GetResult(&Item)))
			{
				PWSTR pszFilePath;
				// Display the file name to the user.
				if (SUCCEEDED(Item->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath)))
				{
					Path = pszFilePath;
					CoTaskMemFree(pszFilePath);
				}
			}
		}
	}

	return Path;
}
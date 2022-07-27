#include "FileSystem.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <ShObjIdl.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

std::filesystem::path FileSystem::OpenDialog(Span<const FilterDesc> FilterSpecs)
{
	// COMDLG_FILTERSPEC ComDlgFS[3] = { { L"C++ code files", L"*.cpp;*.h;*.rc" },
	//								  { L"Executable Files", L"*.exe;*.dll" },
	//								  { L"All Files (*.*)", L"*.*" } };
	std::vector<COMDLG_FILTERSPEC> Specs(FilterSpecs.size());
	for (size_t i = 0; i < Specs.size(); ++i)
	{
		Specs[i].pszName = FilterSpecs[i].Name.data();
		Specs[i].pszSpec = FilterSpecs[i].Filter.data();
	}

	std::filesystem::path	Path;
	ComPtr<IFileOpenDialog> FileOpen;
	if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&FileOpen))))
	{
		FileOpen->SetFileTypes(static_cast<UINT>(Specs.size()), Specs.data());

		// Show the Open dialog box.
		// Get the file name from the dialog box.
		ComPtr<IShellItem> Item;
		PWSTR			   pszFilePath;
		if (SUCCEEDED(FileOpen->Show(nullptr)) &&
			SUCCEEDED(FileOpen->GetResult(&Item)) &&
			SUCCEEDED(Item->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath)))
		{
			// Display the file name to the user.
			Path = pszFilePath;
			CoTaskMemFree(pszFilePath);
		}
	}

	return Path;
}

std::vector<std::filesystem::path> FileSystem::OpenDialogMultiple(Span<const FilterDesc> FilterSpecs)
{
	std::vector<COMDLG_FILTERSPEC> Specs(FilterSpecs.size());
	for (size_t i = 0; i < Specs.size(); ++i)
	{
		Specs[i].pszName = FilterSpecs[i].Name.data();
		Specs[i].pszSpec = FilterSpecs[i].Filter.data();
	}

	std::vector<std::filesystem::path> Paths;
	ComPtr<IFileOpenDialog>			   FileOpen;
	if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&FileOpen))))
	{
		FileOpen->SetFileTypes(static_cast<UINT>(Specs.size()), Specs.data());

		DWORD Options;
		if (SUCCEEDED(FileOpen->GetOptions(&Options)) &&
			SUCCEEDED(FileOpen->SetOptions(Options | FOS_ALLOWMULTISELECT)) &&
			SUCCEEDED(FileOpen->Show(nullptr)))
		{
			ComPtr<IShellItemArray> ItemArray;
			DWORD					Count;
			if (SUCCEEDED(FileOpen->GetResults(&ItemArray)) &&
				SUCCEEDED(ItemArray->GetCount(&Count)))
			{
				Paths.reserve(Count);

				ComPtr<IShellItem> Item;
				PWSTR			   pszFilePath = nullptr;
				for (DWORD Index = 0; Index < Count; ++Index)
				{
					if (SUCCEEDED(ItemArray->GetItemAt(Index, &Item)) &&
						SUCCEEDED(Item->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath)))
					{
						Paths.emplace_back(pszFilePath);
						CoTaskMemFree(pszFilePath);
					}
				}
			}
		}
	}

	return Paths;
}

std::filesystem::path FileSystem::SaveDialog(Span<const FilterDesc> FilterSpecs)
{
	std::vector<COMDLG_FILTERSPEC> Specs(FilterSpecs.size());
	for (size_t i = 0; i < Specs.size(); ++i)
	{
		Specs[i].pszName = FilterSpecs[i].Name.data();
		Specs[i].pszSpec = FilterSpecs[i].Filter.data();
	}

	std::filesystem::path	Path;
	ComPtr<IFileSaveDialog> FileSave;
	if (SUCCEEDED(CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&FileSave))))
	{
		FileSave->SetFileTypes(static_cast<UINT>(Specs.size()), Specs.data());

		// Show the Save dialog box.
		// Get the file name from the dialog box.
		ComPtr<IShellItem> Item;
		PWSTR			   pszFilePath = nullptr;
		if (SUCCEEDED(FileSave->Show(nullptr)) &&
			SUCCEEDED(FileSave->GetResult(&Item)) &&
			SUCCEEDED(Item->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath)))
		{
			// Display the file name to the user.
			Path = pszFilePath;
			CoTaskMemFree(pszFilePath);
		}
	}

	return Path;
}

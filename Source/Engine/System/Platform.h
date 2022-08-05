#pragma once
#include <string_view>
#include "PlatformWindows.h"

class Platform
{
public:
	static void Exit(std::wstring_view Message);

	static void Utf8ToWide(std::string_view Utf8, std::wstring& Wide);
	static void WideToUtf8(std::wstring_view Wide, std::string& Utf8);
};

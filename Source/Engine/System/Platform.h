#pragma once
#include <string_view>
#include "PlatformWindows.h"

class Platform
{
public:
	static void Exit(std::wstring_view Message);
};

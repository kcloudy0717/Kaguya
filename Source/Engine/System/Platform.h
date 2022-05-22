#pragma once
#include <string_view>

class Platform
{
public:
	static void Exit(std::wstring_view Message);
};

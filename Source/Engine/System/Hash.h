#pragma once
#include "Types.h"

class Hash
{
public:
	static u64 Hash64(const void* Object, size_t SizeInBytes);
};

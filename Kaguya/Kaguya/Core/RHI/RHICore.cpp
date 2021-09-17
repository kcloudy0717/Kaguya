#include "RHICore.h"

unsigned long IRHIObject::AddRef()
{
	return ++NumReferences;
}

unsigned long IRHIObject::Release()
{
	unsigned long Result = --NumReferences;
	if (Result == 0)
	{
		delete this;
	}
	return Result;
}

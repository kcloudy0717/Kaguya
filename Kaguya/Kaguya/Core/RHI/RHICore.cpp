#include "RHICore.h"

unsigned long IRHIObject::AddRef()
{
	return ++NumReferences;
}

unsigned long IRHIObject::Release()
{
	unsigned long result = --NumReferences;
	if (result == 0)
	{
		delete this;
	}
	return result;
}

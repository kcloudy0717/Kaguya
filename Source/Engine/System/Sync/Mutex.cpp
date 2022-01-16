#include "Mutex.h"

Mutex::Mutex()
{
	InitializeCriticalSectionEx(&Handle, 0, 0);
}

Mutex::~Mutex()
{
	DeleteCriticalSection(&Handle);
}

void Mutex::Enter()
{
	EnterCriticalSection(&Handle);
}

void Mutex::Leave()
{
	LeaveCriticalSection(&Handle);
}

bool Mutex::TryEnter()
{
	return TryEnterCriticalSection(&Handle);
}

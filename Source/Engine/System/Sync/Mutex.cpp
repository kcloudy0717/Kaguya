#include "Mutex.h"

Mutex::Mutex()
{
	Kaguya::Windows::InitializeCriticalSectionEx(&Handle);
}

Mutex::~Mutex()
{
	Kaguya::Windows::DeleteCriticalSection(&Handle);
}

void Mutex::Enter()
{
	Kaguya::Windows::EnterCriticalSection(&Handle);
}

void Mutex::Leave()
{
	Kaguya::Windows::LeaveCriticalSection(&Handle);
}

bool Mutex::TryEnter()
{
	return Kaguya::Windows::TryEnterCriticalSection(&Handle);
}

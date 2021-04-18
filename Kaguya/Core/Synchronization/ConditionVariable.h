#pragma once
#include <synchapi.h>

class ConditionVariable : public CONDITION_VARIABLE
{
public:
	ConditionVariable()
	{
		::InitializeConditionVariable(this);
	}

	void Wait(CRITICAL_SECTION& CriticalSection, DWORD Milliseconds)
	{
		::SleepConditionVariableCS(this, &CriticalSection, Milliseconds);
	}

	void Wake()
	{
		WakeConditionVariable(this);
	}

	void WakeAll()
	{
		WakeAllConditionVariable(this);
	}
};
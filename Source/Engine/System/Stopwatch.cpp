#include "Stopwatch.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

const i64 Stopwatch::Frequency = []
{
	LARGE_INTEGER Frequency = {};
	QueryPerformanceFrequency(&Frequency);
	return Frequency.QuadPart;
}();

i64 Stopwatch::GetTimestamp()
{
	LARGE_INTEGER Timestamp = {};
	QueryPerformanceCounter(&Timestamp);
	return Timestamp.QuadPart;
}

Stopwatch::Stopwatch() noexcept
	: StartTime(GetTimestamp())
	, Period(1.0 / static_cast<double>(Frequency))
{
}

double Stopwatch::GetDeltaTime() const noexcept
{
	return DeltaTime;
}

double Stopwatch::GetTotalTime() const noexcept
{
	if (Paused)
	{
		return static_cast<double>((StopTime - PausedTime) - TotalTime) * Period;
	}
	return static_cast<double>((CurrentTime - PausedTime) - TotalTime) * Period;
}

double Stopwatch::GetTotalTimePrecise() const noexcept
{
	i64 Now = GetTimestamp();
	return static_cast<double>(Now - StartTime) * Period;
}

bool Stopwatch::IsPaused() const noexcept
{
	return Paused;
}

void Stopwatch::Resume() noexcept
{
	CurrentTime = GetTimestamp();
	if (Paused)
	{
		PausedTime += (CurrentTime - StopTime);

		PreviousTime = CurrentTime;
		StopTime	 = 0;
		Paused		 = false;
	}
}

void Stopwatch::Pause() noexcept
{
	if (!Paused)
	{
		CurrentTime = GetTimestamp();
		StopTime	= CurrentTime;
		Paused		= true;
	}
}

void Stopwatch::Signal() noexcept
{
	if (Paused)
	{
		DeltaTime = 0.0;
		return;
	}
	CurrentTime = GetTimestamp();

	DeltaTime = static_cast<double>(CurrentTime - PreviousTime) * Period;

	PreviousTime = CurrentTime;

	// Clamp the timer to non-negative values to ensure the timer is accurate.
	// DeltaTime can be outside this range if processor goes into a
	// power save mode or we somehow get shuffled to another processor.
	if (DeltaTime < 0.0)
	{
		DeltaTime = 0.0;
	}
}

void Stopwatch::Restart() noexcept
{
	CurrentTime	 = GetTimestamp();
	TotalTime	 = CurrentTime;
	PreviousTime = CurrentTime;
	StopTime	 = 0;
	Paused		 = false;
	Resume();
}

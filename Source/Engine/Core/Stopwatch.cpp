#include "Stopwatch.h"

Stopwatch::Stopwatch() noexcept
{
	LARGE_INTEGER Frequency = {};
	QueryPerformanceFrequency(&Frequency);
	Period = 1.0 / static_cast<double>(Frequency.QuadPart);
	QueryPerformanceCounter(&StartTime);
}

double Stopwatch::GetDeltaTime() const noexcept
{
	return DeltaTime;
}

double Stopwatch::GetTotalTime() const noexcept
{
	if (Paused)
	{
		return static_cast<double>((StopTime.QuadPart - PausedTime.QuadPart) - TotalTime.QuadPart) * Period;
	}
	return static_cast<double>((CurrentTime.QuadPart - PausedTime.QuadPart) - TotalTime.QuadPart) * Period;
}

double Stopwatch::GetTotalTimePrecise() const noexcept
{
	LARGE_INTEGER Now = {};
	QueryPerformanceCounter(&Now);
	return static_cast<double>(Now.QuadPart - StartTime.QuadPart) * Period;
}

void Stopwatch::Resume() noexcept
{
	QueryPerformanceCounter(&CurrentTime);
	if (Paused)
	{
		PausedTime.QuadPart += (CurrentTime.QuadPart - StopTime.QuadPart);

		PreviousTime	  = CurrentTime;
		StopTime.QuadPart = 0;
		Paused			  = false;
	}
}

void Stopwatch::Pause() noexcept
{
	if (!Paused)
	{
		QueryPerformanceCounter(&CurrentTime);
		StopTime = CurrentTime;
		Paused	 = true;
	}
}

void Stopwatch::Signal() noexcept
{
	if (Paused)
	{
		DeltaTime = 0.0;
		return;
	}
	QueryPerformanceCounter(&CurrentTime);

	DeltaTime = static_cast<double>(CurrentTime.QuadPart - PreviousTime.QuadPart) * Period;

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
	QueryPerformanceCounter(&CurrentTime);
	TotalTime		  = CurrentTime;
	PreviousTime	  = CurrentTime;
	StopTime.QuadPart = 0;
	Paused			  = false;
	Resume();
}

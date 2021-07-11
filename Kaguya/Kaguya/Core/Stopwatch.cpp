#include "Stopwatch.h"

Stopwatch::Stopwatch()
{
	::QueryPerformanceFrequency(&Frequency);
	Period = 1.0 / static_cast<double>(Frequency.QuadPart);
	::QueryPerformanceCounter(&StartTime);
}

double Stopwatch::GetDeltaTime() const
{
	return DeltaTime;
}

double Stopwatch::GetTotalTime() const
{
	// If we are stopped, do not count the time that has passed since we stopped.
	// Moreover, if we previously already had a pause, the distance
	// stopTime - totalTime includes paused time, which we do not want to count.
	// To correct this, we can subtract the paused time from stopTime:
	//                     |<--paused time-->|
	// ----*---------------*-----------------*------------*------------*------> time
	//  totalTime       stopTime        startTime     stopTime    currentTime
	if (Paused)
	{
		return ((StopTime.QuadPart - PausedTime.QuadPart) - TotalTime.QuadPart) * Period;
	}
	// The distance currentTime - totalTime includes paused time,
	// which we do not want to count.
	// To correct this, we can subtract the paused time from currentTime:
	// (currentTime - pausedTime) - totalTime
	//                     |<--paused time-->|
	// ----*---------------*-----------------*------------*------> time
	//  totalTime       stopTime        startTime     currentTime
	return ((CurrentTime.QuadPart - PausedTime.QuadPart) - TotalTime.QuadPart) * Period;
}

double Stopwatch::GetTotalTimePrecise() const
{
	LARGE_INTEGER currentTime;
	::QueryPerformanceCounter(&currentTime);
	return (currentTime.QuadPart - StartTime.QuadPart) * Period;
}

void Stopwatch::Resume()
{
	::QueryPerformanceCounter(&CurrentTime);
	// Accumulate the time elapsed between stop and start pairs.
	//                     |<-------d------->|
	// ----*---------------*-----------------*------------> time
	//  totalTime       stopTime        startTime
	if (Paused)
	{
		PausedTime.QuadPart += (CurrentTime.QuadPart - StopTime.QuadPart);

		PreviousTime	  = CurrentTime;
		StopTime.QuadPart = 0;
		Paused			  = false;
	}
}

void Stopwatch::Pause()
{
	if (!Paused)
	{
		::QueryPerformanceCounter(&CurrentTime);
		StopTime = CurrentTime;
		Paused	 = true;
	}
}

void Stopwatch::Signal()
{
	if (Paused)
	{
		DeltaTime = 0.0;
		return;
	}
	::QueryPerformanceCounter(&CurrentTime);
	// Time difference between this frame and the previous.
	DeltaTime = (CurrentTime.QuadPart - PreviousTime.QuadPart) * Period;

	// Prepare for next frame.
	PreviousTime = CurrentTime;

	// Force nonnegative.  The DXSDK's CDXUTTimer mentions that if the
	// processor goes into a power save mode or we get shuffled to another
	// processor, then deltaTime can be negative.
	if (DeltaTime < 0.0)
	{
		DeltaTime = 0.0;
	}
}

void Stopwatch::Restart()
{
	::QueryPerformanceCounter(&CurrentTime);
	TotalTime		  = CurrentTime;
	PreviousTime	  = CurrentTime;
	StopTime.QuadPart = 0;
	Paused			  = false;
	Resume();
}

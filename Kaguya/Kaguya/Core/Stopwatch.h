#pragma once
#include <winnt.h>

class Stopwatch
{
public:
	Stopwatch();

	double GetDeltaTime() const;
	double GetTotalTime() const;
	double GetTotalTimePrecise() const;

	void Resume();
	void Pause();
	void Signal();
	void Restart();

private:
	LARGE_INTEGER Frequency	   = {};
	LARGE_INTEGER StartTime	   = {};
	LARGE_INTEGER TotalTime	   = {};
	LARGE_INTEGER PausedTime   = {};
	LARGE_INTEGER StopTime	   = {};
	LARGE_INTEGER PreviousTime = {};
	LARGE_INTEGER CurrentTime  = {};
	double		  Period	   = 0.0;
	double		  DeltaTime	   = 0.0;

	bool Paused = false;
};

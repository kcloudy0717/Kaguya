#pragma once
#include <winnt.h>

class Stopwatch
{
public:
	Stopwatch() noexcept;

	[[nodiscard]] double GetDeltaTime() const noexcept;
	[[nodiscard]] double GetTotalTime() const noexcept;
	[[nodiscard]] double GetTotalTimePrecise() const noexcept;

	void Resume() noexcept;
	void Pause() noexcept;
	void Signal() noexcept;
	void Restart() noexcept;

private:
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

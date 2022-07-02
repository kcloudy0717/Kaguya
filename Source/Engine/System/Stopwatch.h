#pragma once
#include "Types.h"

class Stopwatch
{
public:
	static i64 GetTimestamp();

	Stopwatch() noexcept;

	[[nodiscard]] f64  GetDeltaTime() const noexcept;
	[[nodiscard]] f64  GetTotalTime() const noexcept;
	[[nodiscard]] f64  GetTotalTimePrecise() const noexcept;
	[[nodiscard]] bool IsPaused() const noexcept;

	void Resume() noexcept;
	void Pause() noexcept;
	void Signal() noexcept;
	void Restart() noexcept;

	static const i64 Frequency; // Ticks per second

private:
	i64 StartTime	 = {};
	i64 TotalTime	 = {};
	i64 PausedTime	 = {};
	i64 StopTime	 = {};
	i64 PreviousTime = {};
	i64 CurrentTime	 = {};
	f64 Period		 = 0.0;
	f64 DeltaTime	 = 0.0;

	bool Paused = false;
};

template<typename TCallback>
class ScopedTimer
{
public:
	ScopedTimer(TCallback&& Callback)
		: Callback(std::move(Callback))
		, Start(Stopwatch::GetTimestamp())
	{
	}
	~ScopedTimer()
	{
		i64 End					= Stopwatch::GetTimestamp();
		i64 Elapsed				= End - Start;
		i64 ElapsedMilliseconds = Elapsed * 1000 / Stopwatch::Frequency;
		Callback(ElapsedMilliseconds);
	}

private:
	TCallback Callback;
	i64		  Start;
};

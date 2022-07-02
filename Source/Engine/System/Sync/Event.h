#pragma once
#include "Types.h"
#include "Platform.h"

enum class EventOptions
{
	None		= 0x0,
	ManualReset = 0x1,
	Signaled	= 0x2
};
ENABLE_BITMASK_OPERATORS(EventOptions);

class Event
{
public:
	static constexpr u32 Infinite = 0xFFFFFFFF;

	Event();
	~Event();

	Event(Event&&) noexcept = default;
	Event& operator=(Event&&) noexcept = default;

	Event(const Event&) = delete;
	Event& operator=(const Event&) = delete;

	[[nodiscard]] ScopedEventHandle::NativeHandle Get() const noexcept;

	void Create(EventOptions Options = EventOptions::None);

	void SetEvent() const noexcept;
	void ResetEvent() const noexcept;

	[[nodiscard]] bool Wait(u32 Milliseconds = Infinite) const noexcept;

private:
	ScopedEventHandle Handle;
};

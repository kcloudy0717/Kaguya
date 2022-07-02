#include "Event.h"
#include <cassert>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

Event::Event()
{
}

Event::~Event()
{
}

ScopedEventHandle::NativeHandle Event::Get() const noexcept
{
	return Handle.Get();
}

void Event::Create(EventOptions Options /*= EventOptions::None*/)
{
	Handle = ScopedEventHandle{
		CreateEventExW(
			nullptr,
			nullptr,
			(EnumMaskBitSet(Options, EventOptions::ManualReset) ? CREATE_EVENT_MANUAL_RESET : 0) | (EnumMaskBitSet(Options, EventOptions::Signaled) ? CREATE_EVENT_INITIAL_SET : 0),
			EVENT_ALL_ACCESS)
	};
	assert(Handle);
}

void Event::SetEvent() const noexcept
{
	::SetEvent(Handle.Get());
}

void Event::ResetEvent() const noexcept
{
	::ResetEvent(Handle.Get());
}

bool Event::Wait(u32 Milliseconds /*= Infinite*/) const noexcept
{
	DWORD Result = ::WaitForSingleObjectEx(Handle.Get(), Milliseconds, FALSE);
	return Result == WAIT_OBJECT_0;
}

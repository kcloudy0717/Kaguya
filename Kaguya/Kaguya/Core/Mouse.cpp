#include "Mouse.h"

bool Mouse::IsPressed(Button Button) const
{
	return ButtonStates[Button];
}

bool Mouse::IsLeftPressed() const
{
	return IsPressed(Left);
}

bool Mouse::IsMiddlePressed() const
{
	return IsPressed(Middle);
}

bool Mouse::IsRightPressed() const
{
	return IsPressed(Right);
}

std::optional<Mouse::Event> Mouse::Read()
{
	if (EventQueue.empty())
	{
		return {};
	}

	Event Event = EventQueue.front();
	EventQueue.pop();
	return Event;
}

std::optional<Mouse::RawInput> Mouse::ReadRawInput()
{
	if (RawInputQueue.empty())
	{
		return {};
	}

	RawInput RawInput = RawInputQueue.front();
	RawInputQueue.pop();
	return RawInput;
}

void Mouse::OnMove(int x, int y)
{
	this->x = x, this->y = y;
	EventQueue.push(Event(Event::EType::Move, *this));
	Trim(EventQueue);
}

void Mouse::OnButtonDown(Button Button, int x, int y)
{
	this->x = x, this->y = y;
	ButtonStates[Button] = true;
	switch (Button)
	{
	case Left:
		EventQueue.push(Event(Event::EType::LMBPress, *this));
		break;
	case Middle:
		EventQueue.push(Event(Event::EType::MMBPress, *this));
		break;
	case Right:
		EventQueue.push(Event(Event::EType::RMBPress, *this));
		break;
	default:
		break;
	}
	Trim(EventQueue);
}

void Mouse::OnButtonUp(Button Button, int x, int y)
{
	this->x = x, this->y = y;
	ButtonStates[Button] = false;
	switch (Button)
	{
	case Mouse::Left:
		EventQueue.push(Mouse::Event(Mouse::Event::EType::LMBRelease, *this));
		break;
	case Mouse::Middle:
		EventQueue.push(Mouse::Event(Mouse::Event::EType::MMBRelease, *this));
		break;
	case Mouse::Right:
		EventQueue.push(Mouse::Event(Mouse::Event::EType::RMBRelease, *this));
		break;
	case Mouse::NumButtons:
		[[fallthrough]];
	default:
		break;
	}
	Trim(EventQueue);
}

void Mouse::OnWheelDown(int x, int y)
{
	this->x = x, this->y = y;
	EventQueue.push(Mouse::Event(Mouse::Event::EType::WheelDown, *this));
	Trim(EventQueue);
}

void Mouse::OnWheelUp(int x, int y)
{
	this->x = x, this->y = y;
	EventQueue.push(Mouse::Event(Mouse::Event::EType::WheelUp, *this));
	Trim(EventQueue);
}

void Mouse::OnWheelDelta(int WheelDelta, int x, int y)
{
	WheelDeltaCarry += WheelDelta;
	// Generate events for every 120
	while (WheelDeltaCarry >= WHEEL_DELTA)
	{
		WheelDeltaCarry -= WHEEL_DELTA;
		OnWheelUp(x, y);
	}
	while (WheelDeltaCarry <= -WHEEL_DELTA)
	{
		WheelDeltaCarry += WHEEL_DELTA;
		OnWheelDown(x, y);
	}
}

void Mouse::OnRawInput(int x, int y)
{
	xRaw = x, yRaw = y;

	RawInputQueue.push({ x, y });
	Trim(RawInputQueue);
}

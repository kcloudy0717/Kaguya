#include "Mouse.h"

bool Mouse::IsPressed(Button Button) const
{
	return ButtonStates[Button];
}

bool Mouse::IsLeftPressed() const
{
	return IsPressed(Button::Left);
}

bool Mouse::IsMiddlePressed() const
{
	return IsPressed(Button::Middle);
}

bool Mouse::IsRightPressed() const
{
	return IsPressed(Button::Right);
}

std::optional<Mouse::Event> Mouse::Read()
{
	if (MouseBuffer.empty())
	{
		return {};
	}

	Mouse::Event e = MouseBuffer.front();
	MouseBuffer.pop();

	return e;
}

std::optional<Mouse::RawInput> Mouse::ReadRawInput()
{
	if (RawDeltaBuffer.empty())
	{
		return {};
	}

	Mouse::RawInput e = RawDeltaBuffer.front();
	RawDeltaBuffer.pop();

	return e;
}

void Mouse::OnMove(int x, int y)
{
	this->x = x, this->y = y;
	MouseBuffer.push(Mouse::Event(Mouse::Event::EType::Move, *this));
	TrimBuffer(MouseBuffer);
}

void Mouse::OnButtonDown(Button Button, int x, int y)
{
	this->x = x, this->y = y;
	ButtonStates[Button] = true;
	switch (Button)
	{
	case Mouse::Left:
		MouseBuffer.push(Mouse::Event(Mouse::Event::EType::LMBPress, *this));
		break;
	case Mouse::Middle:
		MouseBuffer.push(Mouse::Event(Mouse::Event::EType::MMBPress, *this));
		break;
	case Mouse::Right:
		MouseBuffer.push(Mouse::Event(Mouse::Event::EType::RMBPress, *this));
		break;
	case Mouse::NumButtons:
		[[fallthrough]];
	default:
		break;
	}
	TrimBuffer(MouseBuffer);
}

void Mouse::OnButtonUp(Button Button, int x, int y)
{
	this->x = x, this->y = y;
	ButtonStates[Button] = false;
	switch (Button)
	{
	case Mouse::Left:
		MouseBuffer.push(Mouse::Event(Mouse::Event::EType::LMBRelease, *this));
		break;
	case Mouse::Middle:
		MouseBuffer.push(Mouse::Event(Mouse::Event::EType::MMBRelease, *this));
		break;
	case Mouse::Right:
		MouseBuffer.push(Mouse::Event(Mouse::Event::EType::RMBRelease, *this));
		break;
	case Mouse::NumButtons:
		[[fallthrough]];
	default:
		break;
	}
	TrimBuffer(MouseBuffer);
}

void Mouse::OnWheelDown(int x, int y)
{
	this->x = x, this->y = y;
	MouseBuffer.push(Mouse::Event(Mouse::Event::EType::WheelDown, *this));
	TrimBuffer(MouseBuffer);
}

void Mouse::OnWheelUp(int x, int y)
{
	this->x = x, this->y = y;
	MouseBuffer.push(Mouse::Event(Mouse::Event::EType::WheelUp, *this));
	TrimBuffer(MouseBuffer);
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

	RawDeltaBuffer.push({ x, y });
	TrimBuffer(RawDeltaBuffer);
}

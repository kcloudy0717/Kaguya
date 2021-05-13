#include "pch.h"
#include "Mouse.h"

bool Mouse::IsPressed(Button Button) const
{
	return m_ButtonStates[Button];
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

bool Mouse::IsInWindow() const
{
	return m_IsInWindow;
}

std::optional<Mouse::Event> Mouse::Read()
{
	if (Mouse::Event e;
		m_MouseBuffer.pop(e, 0))
	{
		return e;
	}
	return {};
}

std::optional<Mouse::RawInput> Mouse::ReadRawInput()
{
	if (Mouse::RawInput e;
		m_RawDeltaBuffer.pop(e, 0))
	{
		return e;
	}
	return {};
}

void Mouse::OnMove(int x, int y)
{
	this->x = x, this->y = y;
	m_MouseBuffer.push(Mouse::Event(Mouse::Event::EType::Move, *this));
	TrimBuffer(m_MouseBuffer);
}

void Mouse::OnEnter()
{
	m_IsInWindow = true;
	m_MouseBuffer.push(Mouse::Event(Mouse::Event::EType::Enter, *this));
	TrimBuffer(m_MouseBuffer);
}

void Mouse::OnLeave()
{
	m_IsInWindow = false;
	m_MouseBuffer.push(Mouse::Event(Mouse::Event::EType::Leave, *this));
	TrimBuffer(m_MouseBuffer);
}

void Mouse::OnButtonDown(Button Button, int x, int y)
{
	this->x = x, this->y = y;
	m_ButtonStates[Button] = true;
	switch (Button)
	{
	case Mouse::Left: m_MouseBuffer.push(Mouse::Event(Mouse::Event::EType::LMBPress, *this)); break;
	case Mouse::Middle: m_MouseBuffer.push(Mouse::Event(Mouse::Event::EType::MMBPress, *this)); break;
	case Mouse::Right: m_MouseBuffer.push(Mouse::Event(Mouse::Event::EType::RMBPress, *this)); break;
	case Mouse::NumButtons: [[fallthrough]];
	default:
		break;
	}
	TrimBuffer(m_MouseBuffer);
}

void Mouse::OnButtonUp(Button Button, int x, int y)
{
	this->x = x, this->y = y;
	m_ButtonStates[Button] = false;
	switch (Button)
	{
	case Mouse::Left: m_MouseBuffer.push(Mouse::Event(Mouse::Event::EType::LMBRelease, *this)); break;
	case Mouse::Middle: m_MouseBuffer.push(Mouse::Event(Mouse::Event::EType::MMBRelease, *this)); break;
	case Mouse::Right: m_MouseBuffer.push(Mouse::Event(Mouse::Event::EType::RMBRelease, *this)); break;
	case Mouse::NumButtons: [[fallthrough]];
	default:
		break;
	}
	TrimBuffer(m_MouseBuffer);
}

void Mouse::OnWheelDown(int x, int y)
{
	this->x = x, this->y = y;
	m_MouseBuffer.push(Mouse::Event(Mouse::Event::EType::WheelDown, *this));
	TrimBuffer(m_MouseBuffer);
}

void Mouse::OnWheelUp(int x, int y)
{
	this->x = x, this->y = y;
	m_MouseBuffer.push(Mouse::Event(Mouse::Event::EType::WheelUp, *this));
	TrimBuffer(m_MouseBuffer);
}

void Mouse::OnWheelDelta(int WheelDelta, int x, int y)
{
	m_WheelDeltaCarry += WheelDelta;
	// Generate events for every 120 
	while (m_WheelDeltaCarry >= WHEEL_DELTA)
	{
		m_WheelDeltaCarry -= WHEEL_DELTA;
		OnWheelUp(x, y);
	}
	while (m_WheelDeltaCarry <= -WHEEL_DELTA)
	{
		m_WheelDeltaCarry += WHEEL_DELTA;
		OnWheelDown(x, y);
	}
}

void Mouse::OnRawInput(int x, int y)
{
	xRaw = x, yRaw = y;

	m_RawDeltaBuffer.push({ x, y });
	TrimBuffer(m_RawDeltaBuffer);
}

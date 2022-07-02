#include "InputManager.h"
#include "Window.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

bool InputManager::IsPressed(EMouseButton Button) const
{
	return ButtonStates[static_cast<size_t>(Button)];
}

bool InputManager::IsLeftPressed() const
{
	return IsPressed(EMouseButton::Left);
}

bool InputManager::IsMiddlePressed() const
{
	return IsPressed(EMouseButton::Middle);
}

bool InputManager::IsRightPressed() const
{
	return IsPressed(EMouseButton::Right);
}

bool InputManager::IsPressed(unsigned char KeyCode) const
{
	return KeyStates[KeyCode];
}

void InputManager::ResetKeyState()
{
	KeyStates.reset();
}

void InputManager::OnButtonDown(EMouseButton Button)
{
	ButtonStates[static_cast<size_t>(Button)] = true;
}

void InputManager::OnButtonUp(EMouseButton Button)
{
	ButtonStates[static_cast<size_t>(Button)] = false;
}

void InputManager::OnKeyDown(unsigned char KeyCode)
{
	KeyStates[KeyCode] = true;
}

void InputManager::OnKeyUp(unsigned char KeyCode)
{
	KeyStates[KeyCode] = false;
}

void InputManager::EnableCursor()
{
	CursorEnabled = true;
	ShowCursor();
	FreeCursor();
}

void InputManager::DisableCursor(Window* Window)
{
	CursorEnabled = false;
	HideCursor();
	ConfineCursor(Window);
}

void InputManager::ConfineCursor(Window* Window)
{
	HWND HWnd		= Window->GetWindowHandle();
	RECT ClientRect = {};
	::GetClientRect(HWnd, &ClientRect);
	::MapWindowPoints(HWnd, nullptr, reinterpret_cast<POINT*>(&ClientRect), 2);
	::ClipCursor(&ClientRect);
}

void InputManager::FreeCursor()
{
	::ClipCursor(nullptr);
}

void InputManager::ShowCursor()
{
	while (::ShowCursor(TRUE) < 0)
	{
	}
}

void InputManager::HideCursor()
{
	while (::ShowCursor(FALSE) >= 0)
	{
	}
}

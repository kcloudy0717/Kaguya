#include "InputManager.h"

bool InputManager::IsPressed(unsigned char KeyCode) const
{
	return KeyStates[KeyCode];
}

void InputManager::ResetKeyState()
{
	KeyStates.reset();
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

void InputManager::DisableCursor(HWND HWnd)
{
	CursorEnabled = false;
	HideCursor();
	ConfineCursor(HWnd);
}

void InputManager::ConfineCursor(HWND HWnd)
{
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

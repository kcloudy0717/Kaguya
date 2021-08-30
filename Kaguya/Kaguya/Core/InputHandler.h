#pragma once
#include "Mouse.h"
#include "Keyboard.h"

class InputHandler
{
public:
	InputHandler() noexcept = default;
	InputHandler(HWND hWnd);

	void Process(UINT uMsg, WPARAM wParam, LPARAM lParam);

	void EnableCursor();
	void DisableCursor();

	void ConfineCursor();
	void FreeCursor();
	void ShowCursor();
	void HideCursor();

private:
	void HandleRawInput(UINT uMsg, WPARAM wParam, LPARAM lParam);

	void HandleStandardInput(UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
	HWND	 hWnd = nullptr;
	Mouse	 Mouse;
	Keyboard Keyboard;

	bool RawInputEnabled = false;
	bool CursorEnabled	 = true;
};

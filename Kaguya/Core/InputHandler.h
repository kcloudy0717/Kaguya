#pragma once
#include "Mouse.h"
#include "Keyboard.h"

class InputHandler
{
public:
	InputHandler() noexcept = default;
	InputHandler(_In_ HWND hWnd);

	void Handle(_In_ const MSG* pMsg);

	void EnableCursor();
	void DisableCursor();

	void ConfineCursor();
	void FreeCursor();
	void ShowCursor();
	void HideCursor();

private:
	void HandleRawInput(_In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam);

	void HandleStandardInput(_In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam);

public:
	HWND	 hWnd = nullptr;
	Mouse	 Mouse;
	Keyboard Keyboard;

	bool RawInputEnabled = false;
	bool CursorEnabled	 = true;
};

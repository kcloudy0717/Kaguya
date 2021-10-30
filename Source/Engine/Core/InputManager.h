#pragma once
#include <bitset>

class InputManager
{
public:
	[[nodiscard]] bool IsPressed(unsigned char KeyCode) const;

	void ResetKeyState();
	void OnKeyDown(unsigned char KeyCode);
	void OnKeyUp(unsigned char KeyCode);

	void EnableCursor();
	void DisableCursor(HWND HWnd);

	void ConfineCursor(HWND HWnd);
	void FreeCursor();
	void ShowCursor();
	void HideCursor();

public:
	bool RawInputEnabled = false;
	bool CursorEnabled	 = true;
	bool AutoRepeat		 = false;

private:
	static constexpr size_t NumKeyStates = 256;

	std::bitset<NumKeyStates> KeyStates;
};

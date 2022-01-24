#pragma once
#include <bitset>

enum class EMouseButton
{
	Left,
	Middle,
	Right,
	NumButtons
};

constexpr const char* EMouseButtonToString(EMouseButton Button)
{
	switch (Button)
	{
	case EMouseButton::Left:
		return "Left";
	case EMouseButton::Middle:
		return "Middle";
	case EMouseButton::Right:
		return "Right";
	}
	return "<unknown>";
}

class InputManager
{
public:
	[[nodiscard]] bool IsPressed(EMouseButton Button) const;
	[[nodiscard]] bool IsLeftPressed() const;
	[[nodiscard]] bool IsMiddlePressed() const;
	[[nodiscard]] bool IsRightPressed() const;
	[[nodiscard]] bool IsPressed(unsigned char KeyCode) const;

	void ResetKeyState();
	void OnButtonDown(EMouseButton Button);
	void OnButtonUp(EMouseButton Button);
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

	std::bitset<static_cast<size_t>(EMouseButton::NumButtons)> ButtonStates;
	std::bitset<NumKeyStates>								   KeyStates;
};

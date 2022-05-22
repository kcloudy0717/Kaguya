#pragma once
#include <bitset>

class Window;

enum class EMouseButton
{
	Left,
	Middle,
	Right,
	NumButtons
};

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
	void DisableCursor(Window* Window);

	void ConfineCursor(Window* Window);
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

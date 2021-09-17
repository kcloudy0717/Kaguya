#include "InputHandler.h"

#include <hidusage.h>

InputHandler::InputHandler(HWND hWnd)
{
	// Register RAWINPUTDEVICE for handling input
	RAWINPUTDEVICE RIDs[2] = {
		RAWINPUTDEVICE{ HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_MOUSE, RIDEV_INPUTSINK, hWnd },
		RAWINPUTDEVICE{ HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_KEYBOARD, RIDEV_INPUTSINK, hWnd }
	};

	if (!::RegisterRawInputDevices(RIDs, 2, sizeof(RIDs[0])))
	{
		ErrorExit(TEXT("RegisterRawInputDevices"));
	}

	this->hWnd = hWnd;
}

void InputHandler::Process(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (RawInputEnabled)
	{
		HandleRawInput(uMsg, wParam, lParam);
	}
	else
	{
		HandleStandardInput(uMsg, wParam, lParam);
	}
}

void InputHandler::EnableCursor()
{
	CursorEnabled = true;
	ShowCursor();
	FreeCursor();
}

void InputHandler::DisableCursor()
{
	CursorEnabled = false;
	HideCursor();
	ConfineCursor();
}

void InputHandler::ConfineCursor()
{
	RECT ClientRect = {};
	::GetClientRect(hWnd, &ClientRect);
	::MapWindowPoints(hWnd, nullptr, reinterpret_cast<POINT*>(&ClientRect), 2);
	::ClipCursor(&ClientRect);
}

void InputHandler::FreeCursor()
{
	::ClipCursor(nullptr);
}

void InputHandler::ShowCursor()
{
	while (::ShowCursor(TRUE) < 0)
	{
	}
}

void InputHandler::HideCursor()
{
	while (::ShowCursor(FALSE) >= 0)
	{
	}
}

void InputHandler::HandleRawInput(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	Mouse.xRaw = Mouse.yRaw = 0;

	switch (uMsg)
	{
	case WM_INPUT:
	{
		UINT dwSize = 0;
		::GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, nullptr, &dwSize, sizeof(RAWINPUTHEADER));

		auto lpb = static_cast<LPBYTE>(_malloca(dwSize));
		if (!lpb)
		{
			break;
		}

		::GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER));

		auto pRawInput = reinterpret_cast<RAWINPUT*>(lpb);

		switch (pRawInput->header.dwType)
		{
		case RIM_TYPEMOUSE:
		{
			const RAWMOUSE& RawMouse = pRawInput->data.mouse;

			if (RawMouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN)
			{
				Mouse.OnButtonDown(Mouse::Left, RawMouse.lLastX, RawMouse.lLastY);
			}
			if (RawMouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN)
			{
				Mouse.OnButtonDown(Mouse::Middle, RawMouse.lLastX, RawMouse.lLastY);
			}
			if (RawMouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN)
			{
				Mouse.OnButtonDown(Mouse::Right, RawMouse.lLastX, RawMouse.lLastY);
			}

			if (RawMouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP)
			{
				Mouse.OnButtonUp(Mouse::Left, RawMouse.lLastX, RawMouse.lLastY);
			}
			if (RawMouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP)
			{
				Mouse.OnButtonUp(Mouse::Middle, RawMouse.lLastX, RawMouse.lLastY);
			}
			if (RawMouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP)
			{
				Mouse.OnButtonUp(Mouse::Right, RawMouse.lLastX, RawMouse.lLastY);
			}

			Mouse.OnRawInput(RawMouse.lLastX, RawMouse.lLastY);
		}
		break;

		case RIM_TYPEKEYBOARD:
		{
			const RAWKEYBOARD& RawKeyboard = pRawInput->data.keyboard;

			if (RawKeyboard.Message == WM_KEYDOWN || RawKeyboard.Message == WM_SYSKEYDOWN)
			{
				Keyboard.OnKeyDown(static_cast<unsigned char>(RawKeyboard.VKey));
			}

			if (RawKeyboard.Message == WM_KEYUP || RawKeyboard.Message == WM_SYSKEYUP)
			{
				Keyboard.OnKeyUp(static_cast<unsigned char>(RawKeyboard.VKey));
			}
		}
		break;

		default:
			break;
		}

		_freea(lpb);
	}
	break;

	default:
		break;
	}
}

void InputHandler::HandleStandardInput(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_MOUSEMOVE:
	{
		auto [x, y]		= MAKEPOINTS(lParam);
		RECT ClientRect = {};
		::GetClientRect(hWnd, &ClientRect);
		LONG Width	= ClientRect.right - ClientRect.left;
		LONG Height = ClientRect.bottom - ClientRect.top;

		if (x >= 0 && x < Width && y >= 0 && y < Height)
		{
			Mouse.OnMove(x, y);
		}
	}
	break;

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	{
		Mouse::Button Button = {};
		if (uMsg == WM_LBUTTONDOWN)
		{
			Button = Mouse::Left;
		}
		if (uMsg == WM_MBUTTONDOWN)
		{
			Button = Mouse::Middle;
		}
		if (uMsg == WM_RBUTTONDOWN)
		{
			Button = Mouse::Right;
		}

		auto [x, y] = MAKEPOINTS(lParam);
		Mouse.OnButtonDown(Button, x, y);
	}
	break;

	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	{
		Mouse::Button Button = {};
		if (uMsg == WM_LBUTTONUP)
		{
			Button = Mouse::Left;
		}
		if (uMsg == WM_MBUTTONUP)
		{
			Button = Mouse::Middle;
		}
		if (uMsg == WM_RBUTTONUP)
		{
			Button = Mouse::Right;
		}

		auto [x, y] = MAKEPOINTS(lParam);
		Mouse.OnButtonUp(Button, x, y);
	}
	break;

	case WM_MOUSEWHEEL:
	{
		auto [x, y]	   = MAKEPOINTS(lParam);
		int WheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
		Mouse.OnWheelDelta(WheelDelta, x, y);
	}
	break;

	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	{
		if (!(lParam & 0x40000000) || Keyboard.AutoRepeat) // Filter AutoRepeat
		{
			Keyboard.OnKeyDown(static_cast<unsigned char>(wParam));
		}
	}
	break;

	case WM_KEYUP:
	case WM_SYSKEYUP:
	{
		Keyboard.OnKeyUp(static_cast<unsigned char>(wParam));
	}
	break;

	case WM_CHAR:
	{
		Keyboard.OnChar(static_cast<unsigned char>(wParam));
	}
	break;

	case WM_KILLFOCUS:
	{
		Keyboard.ResetKeyState();
	}
	break;

	default:
		break;
	}
}

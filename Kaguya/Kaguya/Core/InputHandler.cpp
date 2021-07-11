#include "InputHandler.h"

#include <hidusage.h>

InputHandler::InputHandler(_In_ HWND hWnd)
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

void InputHandler::Process(_In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
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

void InputHandler::HandleRawInput(_In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
	Mouse.xRaw = Mouse.yRaw = 0;

	switch (uMsg)
	{
	case WM_INPUT:
	{
		UINT dwSize = 0;
		::GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, nullptr, &dwSize, sizeof(RAWINPUTHEADER));

		auto lpb = (LPBYTE)_malloca(dwSize);
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
			const RAWMOUSE& mouse = pRawInput->data.mouse;

			if ((mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN))
			{
				Mouse.OnButtonDown(Mouse::Left, mouse.lLastX, mouse.lLastY);
			}
			if ((mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN))
			{
				Mouse.OnButtonDown(Mouse::Middle, mouse.lLastX, mouse.lLastY);
			}
			if ((mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN))
			{
				Mouse.OnButtonDown(Mouse::Right, mouse.lLastX, mouse.lLastY);
			}

			if ((mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP))
			{
				Mouse.OnButtonUp(Mouse::Left, mouse.lLastX, mouse.lLastY);
			}
			if ((mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP))
			{
				Mouse.OnButtonUp(Mouse::Middle, mouse.lLastX, mouse.lLastY);
			}
			if ((mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP))
			{
				Mouse.OnButtonUp(Mouse::Right, mouse.lLastX, mouse.lLastY);
			}

			Mouse.OnRawInput(mouse.lLastX, mouse.lLastY);
		}
		break;

		case RIM_TYPEKEYBOARD:
		{
			const RAWKEYBOARD& keyboard = pRawInput->data.keyboard;

			if (keyboard.Message == WM_KEYDOWN || keyboard.Message == WM_SYSKEYDOWN)
			{
				Keyboard.OnKeyDown(static_cast<unsigned char>(keyboard.VKey));
			}

			if (keyboard.Message == WM_KEYUP || keyboard.Message == WM_SYSKEYUP)
			{
				Keyboard.OnKeyUp(static_cast<unsigned char>(keyboard.VKey));
			}
		}
		break;
		}

		_freea(lpb);
	}
	break;

	default:
		break;
	}
}

void InputHandler::HandleStandardInput(_In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_MOUSEMOVE:
	{
		const POINTS points = MAKEPOINTS(lParam);
		RECT		 rect	= {};
		::GetClientRect(hWnd, &rect);
		LONG width	= rect.right - rect.left;
		LONG height = rect.bottom - rect.top;

		// Within the range of our window dimension -> log move, and log enter + capture mouse (if not previously in
		// window)
		if (points.x >= 0 && points.x < width && points.y >= 0 && points.y < height)
		{
			Mouse.OnMove(points.x, points.y);
			if (!Mouse.IsInWindow)
			{
				::SetCapture(hWnd);
				Mouse.OnEnter();
			}
		}
		// Outside the range of our window dimension -> log move / maintain capture if button down
		else
		{
			if (wParam & (MK_LBUTTON | MK_RBUTTON))
			{
				Mouse.OnMove(points.x, points.y);
			}
			// button up -> release capture / log event for leaving
			else
			{
				::ReleaseCapture();
				Mouse.OnLeave();
			}
		}
	}
	break;

	case WM_LBUTTONDOWN:
		[[fallthrough]];
	case WM_MBUTTONDOWN:
		[[fallthrough]];
	case WM_RBUTTONDOWN:
	{
		Mouse::Button button = {};
		if ((uMsg == WM_LBUTTONDOWN))
		{
			button = Mouse::Left;
		}
		if ((uMsg == WM_MBUTTONDOWN))
		{
			button = Mouse::Middle;
		}
		if ((uMsg == WM_RBUTTONDOWN))
		{
			button = Mouse::Right;
		}

		const POINTS pt = MAKEPOINTS(lParam);
		Mouse.OnButtonDown(button, pt.x, pt.y);
	}
	break;

	case WM_LBUTTONUP:
		[[fallthrough]];
	case WM_MBUTTONUP:
		[[fallthrough]];
	case WM_RBUTTONUP:
	{
		Mouse::Button button = {};
		if ((uMsg == WM_LBUTTONUP))
		{
			button = Mouse::Left;
		}
		if ((uMsg == WM_MBUTTONUP))
		{
			button = Mouse::Middle;
		}
		if ((uMsg == WM_RBUTTONUP))
		{
			button = Mouse::Right;
		}

		const POINTS pt = MAKEPOINTS(lParam);
		Mouse.OnButtonUp(button, pt.x, pt.y);
	}
	break;

	case WM_MOUSEWHEEL:
	{
		const POINTS pt	   = MAKEPOINTS(lParam);
		const int	 delta = GET_WHEEL_DELTA_WPARAM(wParam);
		Mouse.OnWheelDelta(delta, pt.x, pt.y);
	}
	break;

	// Keyboard messages
	case WM_KEYDOWN:
		[[fallthrough]];
		// syskey commands need to be handled to track ALT key (VK_MENU) and F10
	case WM_SYSKEYDOWN:
	{
		if (!(lParam & 0x40000000) || Keyboard.AutoRepeat) // Filter AutoRepeat
		{
			Keyboard.OnKeyDown(static_cast<unsigned char>(wParam));
		}
	}
	break;

	case WM_KEYUP:
		[[fallthrough]];
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

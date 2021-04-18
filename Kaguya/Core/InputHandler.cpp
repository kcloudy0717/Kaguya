#include "pch.h"
#include "InputHandler.h"

#include <hidusage.h>

#include "Window.h"

void InputHandler::Create(Window* pWindow)
{
	m_pWindow = pWindow;

	// Register RAWINPUTDEVICE for handling input
	RAWINPUTDEVICE rid[2] = {};

	rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
	rid[0].usUsage = HID_USAGE_GENERIC_MOUSE;
	rid[0].dwFlags = RIDEV_INPUTSINK;
	rid[0].hwndTarget = pWindow->GetWindowHandle();

	rid[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
	rid[1].usUsage = HID_USAGE_GENERIC_KEYBOARD;
	rid[1].dwFlags = RIDEV_INPUTSINK;
	rid[1].hwndTarget = pWindow->GetWindowHandle();

	if (!::RegisterRawInputDevices(rid, 2, sizeof(rid[0])))
	{
		LOG_ERROR("RegisterRawInputDevices Error: {}", ::GetLastError());
	}
}

void InputHandler::Handle(const MSG* pMsg)
{
	switch (pMsg->message)
	{
	case WM_LBUTTONDOWN: [[fallthrough]];
	case WM_MBUTTONDOWN: [[fallthrough]];
	case WM_RBUTTONDOWN:
	{
		Mouse::Button button = {};
		if ((pMsg->message == WM_LBUTTONDOWN)) { button = Mouse::Left; }
		if ((pMsg->message == WM_MBUTTONDOWN)) { button = Mouse::Middle; }
		if ((pMsg->message == WM_RBUTTONDOWN)) { button = Mouse::Right; }

		Mouse.OnMouseButtonDown(button);
	}
	break;

	case WM_LBUTTONUP: [[fallthrough]];
	case WM_MBUTTONUP: [[fallthrough]];
	case WM_RBUTTONUP:
	{
		Mouse::Button button = {};
		if ((pMsg->message == WM_LBUTTONUP)) { button = Mouse::Left; }
		if ((pMsg->message == WM_MBUTTONUP)) { button = Mouse::Middle; }
		if ((pMsg->message == WM_RBUTTONUP)) { button = Mouse::Right; }

		Mouse.OnMouseButtonUp(button);
	}
	break;

	case WM_KILLFOCUS:
	{
		Keyboard.ResetKeyState();
	}
	break;

	case WM_INPUT:
	{
		if (!Mouse.UseRawInput)
		{
			break;
		}

		UINT dwSize = 0;
		::GetRawInputData(reinterpret_cast<HRAWINPUT>(pMsg->lParam), RID_INPUT, NULL, &dwSize,
			sizeof(RAWINPUTHEADER));

		auto lpb = (LPBYTE)_malloca(dwSize);
		if (!lpb)
		{
			break;
		}

		::GetRawInputData(reinterpret_cast<HRAWINPUT>(pMsg->lParam), RID_INPUT, lpb, &dwSize,
			sizeof(RAWINPUTHEADER));

		auto pRawInput = reinterpret_cast<RAWINPUT*>(lpb);

		switch (pRawInput->header.dwType)
		{
		case RIM_TYPEMOUSE:
		{
			const auto& mouse = pRawInput->data.mouse;

			if ((mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN)) { Mouse.OnMouseButtonDown(Mouse::Left); }
			if ((mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN)) { Mouse.OnMouseButtonDown(Mouse::Middle); }
			if ((mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN)) { Mouse.OnMouseButtonDown(Mouse::Right); }

			if ((mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP)) { Mouse.OnMouseButtonUp(Mouse::Left); }
			if ((mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP)) { Mouse.OnMouseButtonUp(Mouse::Middle); }
			if ((mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP)) { Mouse.OnMouseButtonUp(Mouse::Right); }

			Mouse.OnRawInput(mouse.lLastX, mouse.lLastY);
		}
		break;

		case RIM_TYPEKEYBOARD:
		{
			const auto& keyboard = pRawInput->data.keyboard;

			if (keyboard.Message == WM_KEYDOWN || keyboard.Message == WM_SYSKEYDOWN)
			{
				Keyboard.OnKeyDown(keyboard.VKey);
			}

			if (keyboard.Message == WM_KEYUP || keyboard.Message == WM_SYSKEYUP)
			{
				Keyboard.OnKeyUp(keyboard.VKey);
			}
		}
		break;
		}

		_freea(lpb);
	}
	break;

	case WM_MOUSEMOVE:
	{
		const POINTS points = MAKEPOINTS(pMsg->lParam);

		// Within the range of our window dimension -> log move, and log enter + capture mouse (if not previously in window)
		if (points.x >= 0 && points.x < m_pWindow->GetWindowWidth() && points.y >= 0 && points.y < m_pWindow->GetWindowHeight())
		{
			Mouse.OnMouseMove(points.x, points.y);
			if (!Mouse.IsInWindow())
			{
				::SetCapture(m_pWindow->GetWindowHandle());
				Mouse.OnMouseEnter();
			}
		}
		// Outside the range of our window dimension -> log move / maintain capture if button down
		else
		{
			if (pMsg->wParam & (MK_LBUTTON | MK_RBUTTON))
			{
				Mouse.OnMouseMove(points.x, points.y);
			}
			// button up -> release capture / log event for leaving
			else
			{
				::ReleaseCapture();
				Mouse.OnMouseLeave();
			}
		}
	}
	break;
	}
}
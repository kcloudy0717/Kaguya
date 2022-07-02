#include "Application.h"
#include "IApplicationMessageHandler.h"
#include "Platform.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <ShObjIdl.h>

Application::PlatformWindows::PlatformWindows()
	: Initialized(SUCCEEDED(CoInitializeEx(nullptr, COINIT_MULTITHREADED)))
{
}

Application::PlatformWindows::~PlatformWindows()
{
	if (Initialized)
	{
		CoUninitialize();
	}
}

Application::Application()
	: HInstance(GetModuleHandle(nullptr))
{
	// Register window class
	const WNDCLASSEXW ClassDesc = {
		.cbSize		   = sizeof(WNDCLASSEX),
		.style		   = CS_DBLCLKS,
		.lpfnWndProc   = WindowProc,
		.cbClsExtra	   = 0,
		.cbWndExtra	   = 0,
		.hInstance	   = HInstance,
		.hIcon		   = nullptr,
		.hCursor	   = ::LoadCursor(nullptr, IDC_ARROW),
		.hbrBackground = nullptr,
		.lpszMenuName  = nullptr,
		.lpszClassName = Window::WindowClass.data(),
		.hIconSm	   = nullptr
	};
	if (!RegisterClassExW(&ClassDesc))
	{
		Platform::Exit(L"RegisterClassExW");
	}
}

Application::~Application()
{
	UnregisterClassW(Window::WindowClass.data(), HInstance);
}

void Application::Run()
{
	Initialized = Initialize();

	while (true)
	{
		// Pump messages
		MSG Msg = {};
		while (PeekMessage(&Msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}
		//

		if (RequestExit)
		{
			break;
		}

		if (!Minimized)
		{
			Update();
		}
	}

	Shutdown();
}

void Application::SetMessageHandler(IApplicationMessageHandler* MessageHandler)
{
	this->MessageHandler = MessageHandler;
}

void Application::AddWindow(Window* Parent, Window* Window, const WindowDesc& Desc)
{
	Windows.push_back(Window);
	Window->Initialize(this, Parent, HInstance, Desc);
}

void Application::SetRawInputMode(bool Enable, Window* Window)
{
	Window->SetRawInput(Enable);
}

LRESULT WINDOWS_CALLING_CONVENTION Application::WindowProc(HWND HWnd, u32 Message, WPARAM WParam, LPARAM LParam)
{
	Application* WindowsApplication;
	if (Message == WM_NCCREATE)
	{
		// Save the Application* passed in to CreateWindow.
		auto CreateStruct  = std::bit_cast<LPCREATESTRUCT>(LParam);
		WindowsApplication = static_cast<Application*>(CreateStruct->lpCreateParams);
		SetWindowLongPtr(HWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(WindowsApplication));
	}
	else
	{
		WindowsApplication = reinterpret_cast<Application*>(GetWindowLongPtr(HWnd, GWLP_USERDATA));
	}

	if (WindowsApplication)
	{
		return WindowsApplication->ProcessMessage(HWnd, Message, WParam, LParam);
	}

	return DefWindowProc(HWnd, Message, WParam, LParam);
}

LRESULT Application::ProcessMessage(HWND HWnd, u32 Message, WPARAM WParam, LPARAM LParam)
{
	auto WindowIter = std::ranges::find_if(
		Windows,
		[=](Window* Window)
		{
			return Window->GetWindowHandle() == HWnd;
		});
	if (WindowIter == Windows.end())
	{
		return DefWindowProc(HWnd, Message, WParam, LParam);
	}
	Window* CurrentWindow = *WindowIter;

	Callbacks(CurrentWindow->GetWindowHandle(), Message, WParam, LParam);

	switch (Message)
	{
	case WM_GETMINMAXINFO: // Catch this message so to prevent the window from becoming too small.
	{
		auto Info			 = std::bit_cast<MINMAXINFO*>(LParam);
		Info->ptMinTrackSize = { GetSystemMetrics(SM_CXMINTRACK), GetSystemMetrics(SM_CYMINTRACK) };
	}
	break;

	case WM_KILLFOCUS:
	{
		InputManager.ResetKeyState();
	}
	break;

	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
	{
		auto VirtualKey = static_cast<unsigned char>(WParam);
		bool IsRepeat	= (LParam & 0x40000000) != 0;

		MessageHandler->OnKeyDown(VirtualKey, IsRepeat);
		if (!(LParam & 0x40000000) || InputManager.AutoRepeat) // Filter AutoRepeat
		{
			InputManager.OnKeyDown(VirtualKey);
		}
	}
	break;

	case WM_SYSKEYUP:
	case WM_KEYUP:
	{
		auto VirtualKey = static_cast<unsigned char>(WParam);
		MessageHandler->OnKeyUp(VirtualKey);
		InputManager.OnKeyUp(VirtualKey);
	}
	break;

	case WM_CHAR:
	{
		auto Character = static_cast<unsigned char>(WParam);
		bool IsRepeat  = (LParam & 0x40000000) != 0;
		MessageHandler->OnKeyChar(Character, IsRepeat);
	}
	break;

	case WM_MOUSEMOVE:
	{
		auto [x, y]		= MAKEPOINTS(LParam);
		RECT ClientRect = {};
		::GetClientRect(HWnd, &ClientRect);
		LONG Width	= ClientRect.right - ClientRect.left;
		LONG Height = ClientRect.bottom - ClientRect.top;

		if (x >= 0 && x < Width && y >= 0 && y < Height)
		{
			MessageHandler->OnMouseMove(x, y);
		}
	}
	break;

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	{
		auto Button = EMouseButton::NumButtons;
		if (Message == WM_LBUTTONDOWN)
		{
			Button = EMouseButton::Left;
		}
		if (Message == WM_MBUTTONDOWN)
		{
			Button = EMouseButton::Middle;
		}
		if (Message == WM_RBUTTONDOWN)
		{
			Button = EMouseButton::Right;
		}

		auto [x, y] = MAKEPOINTS(LParam);
		MessageHandler->OnMouseDown(CurrentWindow, Button, x, y);
		InputManager.OnButtonDown(Button);
	}
	break;

	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	{
		auto Button = EMouseButton::NumButtons;
		if (Message == WM_LBUTTONUP)
		{
			Button = EMouseButton::Left;
		}
		if (Message == WM_MBUTTONUP)
		{
			Button = EMouseButton::Middle;
		}
		if (Message == WM_RBUTTONUP)
		{
			Button = EMouseButton::Right;
		}

		auto [x, y] = MAKEPOINTS(LParam);
		MessageHandler->OnMouseUp(Button, x, y);
		InputManager.OnButtonUp(Button);
	}
	break;

	case WM_LBUTTONDBLCLK:
	case WM_MBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
	{
		auto Button = EMouseButton::NumButtons;
		if (Message == WM_LBUTTONDBLCLK)
		{
			Button = EMouseButton::Left;
		}
		if (Message == WM_MBUTTONDBLCLK)
		{
			Button = EMouseButton::Middle;
		}
		if (Message == WM_RBUTTONDBLCLK)
		{
			Button = EMouseButton::Right;
		}

		auto [x, y] = MAKEPOINTS(LParam);
		MessageHandler->OnMouseDoubleClick(CurrentWindow, Button, x, y);
	}
	break;

	case WM_MOUSEWHEEL:
	{
		static constexpr float WheelScale = 1.0f / 120.0f;
		int					   WheelDelta = GET_WHEEL_DELTA_WPARAM(WParam);

		auto [x, y] = MAKEPOINTS(LParam);
		MessageHandler->OnMouseWheel(static_cast<float>(WheelDelta) * WheelScale, x, y);
	}
	break;

	case WM_INPUT:
	{
		if (!CurrentWindow->IsUsingRawInput())
		{
			break;
		}

		UINT Size = 0;
		GetRawInputData(reinterpret_cast<HRAWINPUT>(LParam), RID_INPUT, nullptr, &Size, sizeof(RAWINPUTHEADER));

		if (!Size)
		{
			break;
		}

		auto Buffer = std::make_unique<BYTE[]>(Size);
		GetRawInputData(reinterpret_cast<HRAWINPUT>(LParam), RID_INPUT, Buffer.get(), &Size, sizeof(RAWINPUTHEADER));

		auto RawInput = reinterpret_cast<RAWINPUT*>(Buffer.get());

		switch (RawInput->header.dwType)
		{
		case RIM_TYPEMOUSE:
		{
			const RAWMOUSE& RawMouse = RawInput->data.mouse;

			// bool IsAbsolute = (RawMouse.usFlags & MOUSE_MOVE_ABSOLUTE) == MOUSE_MOVE_ABSOLUTE;
			// if (IsAbsolute)
			// {
			// 	bool IsVirtualDesktop = (RawMouse.usFlags & MOUSE_VIRTUAL_DESKTOP) == MOUSE_VIRTUAL_DESKTOP;
			//
			// 	int Width  = GetSystemMetrics(IsVirtualDesktop ? SM_CXVIRTUALSCREEN : SM_CXSCREEN);
			// 	int Height = GetSystemMetrics(IsVirtualDesktop ? SM_CYVIRTUALSCREEN : SM_CYSCREEN);
			//
			// 	auto AbsoluteX = static_cast<int>((RawMouse.lLastX / 65535.0f) * Width);
			// 	auto AbsoluteY = static_cast<int>((RawMouse.lLastY / 65535.0f) * Height);
			// }
			int RelativeX = RawMouse.lLastX;
			int RelativeY = RawMouse.lLastY;
			MessageHandler->OnRawMouseMove(RelativeX, RelativeY);
		}
		break;

		default:
			break;
		}
	}
	break;

	// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
	case WM_ENTERSIZEMOVE:
	{
		Resizing = true;
	}
	break;

	// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
	case WM_EXITSIZEMOVE:
	{
		Resizing = false;
		if (Initialized)
		{
			MessageHandler->OnWindowResize(CurrentWindow, CurrentWindow->GetWidth(), CurrentWindow->GetHeight());
		}
	}
	break;

	case WM_SIZE:
	{
		int WindowWidth	 = LOWORD(LParam);
		int WindowHeight = HIWORD(LParam);
		CurrentWindow->Resize(WindowWidth, WindowHeight);

		bool ShouldResize = false;

		if (WParam == SIZE_MINIMIZED)
		{
			Minimized = true;
			Maximized = false;
		}
		else if (WParam == SIZE_MAXIMIZED)
		{
			Minimized	 = false;
			Maximized	 = true;
			ShouldResize = true;
		}
		else if (WParam == SIZE_RESTORED)
		{
			// Restoring from minimized state?
			if (Minimized)
			{
				Minimized	 = false;
				ShouldResize = true;
			}
			// Restoring from maximized state?
			else if (Maximized)
			{
				Maximized	 = false;
				ShouldResize = true;
			}
			else if (Resizing)
			{
				// If user is dragging the resize bars, we do not resize
				// the buffers here because as the user continuously
				// drags the resize bars, a stream of WM_SIZE messages are
				// sent to the window, and it would be pointless (and slow)
				// to resize for each WM_SIZE message received from dragging
				// the resize bars.  So instead, we reset after the user is
				// done resizing the window and releases the resize bars, which
				// sends a WM_EXITSIZEMOVE message.
			}
			// API call such as SetWindowPos or IDXGISwapChain::SetFullscreenState
			else
			{
				ShouldResize = true;
			}
		}

		if (ShouldResize)
		{
			if (Initialized)
			{
				MessageHandler->OnWindowResize(CurrentWindow, WindowWidth, WindowHeight);
			}
		}
	}
	break;

	case WM_MOVE:
	{
		if (Initialized)
		{
			MessageHandler->OnWindowMove(CurrentWindow, LOWORD(LParam), HIWORD(LParam));
		}
	}
	break;

	case WM_CLOSE:
	{
		MessageHandler->OnWindowClose(CurrentWindow);
		CurrentWindow->Destroy();
	}
	break;

	case WM_DESTROY:
	{
		Windows.erase(WindowIter);
	}
	break;

	default:
		return DefWindowProc(HWnd, Message, WParam, LParam);
	}

	return 0;
}

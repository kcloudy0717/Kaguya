#include "Application.h"
#include "IApplicationMessageHandler.h"

Application::Application()
	: HInstance(GetModuleHandle(nullptr))
{
	// Register window class
	WNDCLASSEXW ClassDesc	= {};
	ClassDesc.cbSize		= sizeof(WNDCLASSEX);
	ClassDesc.style			= CS_DBLCLKS;
	ClassDesc.lpfnWndProc	= WindowProc;
	ClassDesc.cbClsExtra	= 0;
	ClassDesc.cbWndExtra	= 0;
	ClassDesc.hInstance		= HInstance;
	ClassDesc.hIcon			= nullptr;
	ClassDesc.hCursor		= ::LoadCursor(nullptr, IDC_ARROW);
	ClassDesc.hbrBackground = nullptr;
	ClassDesc.lpszMenuName	= nullptr;
	ClassDesc.lpszClassName = Window::WindowClass;
	ClassDesc.hIconSm		= nullptr;
	if (!RegisterClassExW(&ClassDesc))
	{
		ErrorExit(TEXT("RegisterClassExW"));
	}
}

Application::~Application()
{
	UnregisterClassW(Window::WindowClass, HInstance);
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

void Application::AddWindow(Window* Parent, Window* Window, const WINDOW_DESC& Desc)
{
	Windows.push_back(Window);
	Window->Initialize(this, Parent, HInstance, Desc);
}

void Application::RegisterMessageCallback(MessageCallback Callback, void* Context)
{
	Callbacks.emplace_back(Callback, Context);
}

void Application::SetRawInputMode(bool Enable, Window* Window)
{
	Window->SetRawInput(Enable);
}

LRESULT CALLBACK Application::WindowProc(_In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
	Application* WindowsApplication;
	if (uMsg == WM_NCCREATE)
	{
		// Save the Application* passed in to CreateWindow.
		auto CreateStruct  = std::bit_cast<LPCREATESTRUCT>(lParam);
		WindowsApplication = static_cast<Application*>(CreateStruct->lpCreateParams);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(WindowsApplication));
	}
	else
	{
		WindowsApplication = reinterpret_cast<Application*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	}

	if (WindowsApplication)
	{
		return WindowsApplication->ProcessMessage(hWnd, uMsg, wParam, lParam);
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT Application::ProcessMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	auto WindowIter = std::ranges::find_if(
		Windows,
		[=](Window* Window)
		{
			return Window->GetWindowHandle() == hWnd;
		});
	if (WindowIter == Windows.end())
	{
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	Window* CurrentWindow = *WindowIter;

	for (auto& Callback : Callbacks)
	{
		Callback.Function(Callback.Context, CurrentWindow->GetWindowHandle(), uMsg, wParam, lParam);
	}

	switch (uMsg)
	{
	case WM_GETMINMAXINFO: // Catch this message so to prevent the window from becoming too small.
	{
		auto Info			 = std::bit_cast<MINMAXINFO*>(lParam);
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
		auto VirtualKey = static_cast<unsigned char>(wParam);
		bool IsRepeat	= (lParam & 0x40000000) != 0;

		MessageHandler->OnKeyDown(VirtualKey, IsRepeat);
		if (!(lParam & 0x40000000) || InputManager.AutoRepeat) // Filter AutoRepeat
		{
			InputManager.OnKeyDown(VirtualKey);
		}
	}
	break;

	case WM_SYSKEYUP:
	case WM_KEYUP:
	{
		auto VirtualKey = static_cast<unsigned char>(wParam);
		MessageHandler->OnKeyUp(VirtualKey);
		InputManager.OnKeyUp(VirtualKey);
	}
	break;

	case WM_CHAR:
	{
		auto Character = static_cast<unsigned char>(wParam);
		bool IsRepeat  = (lParam & 0x40000000) != 0;
		MessageHandler->OnKeyChar(Character, IsRepeat);
	}
	break;

	case WM_MOUSEMOVE:
	{
		auto [x, y]		= MAKEPOINTS(lParam);
		RECT ClientRect = {};
		::GetClientRect(hWnd, &ClientRect);
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
		if (uMsg == WM_LBUTTONDOWN)
		{
			Button = EMouseButton::Left;
		}
		if (uMsg == WM_MBUTTONDOWN)
		{
			Button = EMouseButton::Middle;
		}
		if (uMsg == WM_RBUTTONDOWN)
		{
			Button = EMouseButton::Right;
		}

		auto [x, y] = MAKEPOINTS(lParam);
		MessageHandler->OnMouseDown(CurrentWindow, Button, x, y);
		InputManager.OnButtonDown(Button);
	}
	break;

	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	{
		auto Button = EMouseButton::NumButtons;
		if (uMsg == WM_LBUTTONUP)
		{
			Button = EMouseButton::Left;
		}
		if (uMsg == WM_MBUTTONUP)
		{
			Button = EMouseButton::Middle;
		}
		if (uMsg == WM_RBUTTONUP)
		{
			Button = EMouseButton::Right;
		}

		auto [x, y] = MAKEPOINTS(lParam);
		MessageHandler->OnMouseUp(Button, x, y);
		InputManager.OnButtonUp(Button);
	}
	break;

	case WM_LBUTTONDBLCLK:
	case WM_MBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
	{
		auto Button = EMouseButton::NumButtons;
		if (uMsg == WM_LBUTTONDBLCLK)
		{
			Button = EMouseButton::Left;
		}
		if (uMsg == WM_MBUTTONDBLCLK)
		{
			Button = EMouseButton::Middle;
		}
		if (uMsg == WM_RBUTTONDBLCLK)
		{
			Button = EMouseButton::Right;
		}

		auto [x, y] = MAKEPOINTS(lParam);
		MessageHandler->OnMouseDoubleClick(CurrentWindow, Button, x, y);
	}
	break;

	case WM_MOUSEWHEEL:
	{
		static constexpr float WheelScale = 1.0f / 120.0f;
		int					   WheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);

		auto [x, y] = MAKEPOINTS(lParam);
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
		GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, nullptr, &Size, sizeof(RAWINPUTHEADER));

		if (!Size)
		{
			break;
		}

		auto Buffer = std::make_unique<BYTE[]>(Size);
		GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, Buffer.get(), &Size, sizeof(RAWINPUTHEADER));

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
	// Here we reset everything based on the new window dimensions.
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
		int WindowWidth	 = LOWORD(lParam);
		int WindowHeight = HIWORD(lParam);
		CurrentWindow->Resize(WindowWidth, WindowHeight);

		bool ShouldResize = false;

		if (wParam == SIZE_MINIMIZED)
		{
			Minimized = true;
			Maximized = false;
		}
		else if (wParam == SIZE_MAXIMIZED)
		{
			Minimized	 = false;
			Maximized	 = true;
			ShouldResize = true;
		}
		else if (wParam == SIZE_RESTORED)
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
			MessageHandler->OnWindowMove(CurrentWindow, LOWORD(lParam), HIWORD(lParam));
		}
	}
	break;

	case WM_CLOSE:
	{
		MessageHandler->OnWindowClose(CurrentWindow);
	}
	break;

	case WM_DESTROY:
	{
		Windows.erase(WindowIter);
	}
	break;

	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return 0;
}

#include "Application.h"

#include "IApplicationMessageHandler.h"

#include <shellapi.h>

#pragma comment(lib, "runtimeobject.lib")

using Microsoft::WRL::ComPtr;

Application::Application(const std::string& LoggerName, const ApplicationOptions& Options)
	: InitializeWrapper(RO_INIT_MULTITHREADED)
	, HInstance(GetModuleHandle(nullptr))
{
	// Initialize ExecutableDirectory
	int Argc;
	if (LPWSTR* Argv = CommandLineToArgvW(GetCommandLineW(), &Argc); Argv)
	{
		ExecutableDirectory = std::filesystem::path(Argv[0]).parent_path();
		LocalFree(Argv);
	}

	// Initialize Log
	Log::Initialize(LoggerName);
	LOG_INFO("Log Initialized");

	if (!Options.Icon.empty())
	{
		assert(Options.Icon.extension() == ".ico");
		HIcon = wil::unique_hicon(static_cast<HICON>(
			LoadImage(nullptr, Options.Icon.wstring().data(), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_LOADFROMFILE)));
	}

	HCursor = wil::unique_hcursor(::LoadCursor(nullptr, IDC_ARROW));

	// Register window class
	WNDCLASSEXW ClassDesc	= {};
	ClassDesc.cbSize		= sizeof(WNDCLASSEX);
	ClassDesc.style			= CS_DBLCLKS;
	ClassDesc.lpfnWndProc	= WindowProc;
	ClassDesc.cbClsExtra	= 0;
	ClassDesc.cbWndExtra	= 0;
	ClassDesc.hInstance		= HInstance;
	ClassDesc.hIcon			= HIcon.get();
	ClassDesc.hCursor		= HCursor.get();
	ClassDesc.hbrBackground = nullptr;
	ClassDesc.lpszMenuName	= nullptr;
	ClassDesc.lpszClassName = WindowClass;
	ClassDesc.hIconSm		= HIcon.get();
	if (!RegisterClassExW(&ClassDesc))
	{
		ErrorExit(TEXT("RegisterClassExW"));
	}
}

Application::~Application()
{
	UnregisterClassW(WindowClass, HInstance);
}

void Application::Run()
{
	Initialized = Initialize();

	Stopwatch.Restart();
	while (!RequestExit)
	{
		PumpMessages();

		Stopwatch.Signal();
		if (!Minimized)
		{
			DeltaTime = static_cast<float>(Stopwatch.GetDeltaTime());
			Update(DeltaTime);
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
	LOG_INFO(L"Added Window: Address: {}, Name: {}", fmt::ptr(Window), Window->GetDesc().Name);
}

void Application::SetRawInputMode(bool Enable, Window* Window)
{
	Window->SetRawInput(Enable);
}

LRESULT CALLBACK Application::WindowProc(_In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
	Application* WindowsApplication = nullptr;
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

void Application::PumpMessages()
{
	MSG Msg = {};
	while (PeekMessage(&Msg, nullptr, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT Application::ProcessMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
	{
		return true;
	}

	auto WindowIter = std::ranges::find_if(
		Windows.begin(),
		Windows.end(),
		[=](Window* Window)
		{
			return Window->GetWindowHandle() == hWnd;
		});
	if (WindowIter == Windows.end())
	{
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	Window* CurrentWindow = *WindowIter;

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
			MessageHandler->OnMouseMove(Vector2i(x, y));
		}
	}
	break;

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	{
		auto Button = EMouseButton::Unknown;
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
		MessageHandler->OnMouseDown(CurrentWindow, Button, Vector2i(x, y));
	}
	break;

	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	{
		auto Button = EMouseButton::Unknown;
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
		MessageHandler->OnMouseUp(Button, Vector2i(x, y));
	}
	break;

	case WM_LBUTTONDBLCLK:
	case WM_MBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
	{
		auto Button = EMouseButton::Unknown;
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
		MessageHandler->OnMouseDoubleClick(CurrentWindow, Button, Vector2i(x, y));
	}
	break;

	case WM_MOUSEWHEEL:
	{
		static constexpr float WheelScale = 1.0f / 120.0f;
		int					   WheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);

		auto [x, y] = MAKEPOINTS(lParam);
		MessageHandler->OnMouseWheel(static_cast<float>(WheelDelta) * WheelScale, Vector2i(x, y));
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
			MessageHandler->OnRawMouseMove(Vector2i(RelativeX, RelativeY));
		}
		break;

		default:
			break;
		}
	}
	break;

	case WM_SIZE:
	{
		if (wParam == SIZE_MINIMIZED)
		{
			break;
		}

		int WindowWidth	 = LOWORD(lParam);
		int WindowHeight = HIWORD(lParam);

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
			// API call such as SetWindowPos or IDXGISwapChain::SetFullscreenState
			else if (!Resizing)
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

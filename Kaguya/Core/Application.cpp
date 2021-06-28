#include "pch.h"
#include "Application.h"

#if defined(_DEBUG)
// memory leak
#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>
#define ENABLE_LEAK_DETECTION() _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF)
#define SET_LEAK_BREAKPOINT(x)	_CrtSetBreakAlloc(x)
#else
#define ENABLE_LEAK_DETECTION() 0
#define SET_LEAK_BREAKPOINT(X)	X
#endif

#include <shellapi.h>

#pragma comment(lib, "runtimeobject.lib")

// https://www.gamedev.net/forums/topic/693260-vk_snapshot-and-keydown/
// All credit goes to Wicked Engine, I can't for the sake of me figure out how VK_SNAPSHOT works, seems like WM_KEYDOWN
// doesnt generate keycode
enum Hotkeys
{
	UNUSED		= 0,
	PRINTSCREEN = 1,
};

class ImGuiContextManager
{
public:
	ImGuiContextManager(HWND hWnd)
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::StyleColorsDark();
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

		// Initialize ImGui for win32
		ImGui_ImplWin32_Init(hWnd);
	}

	~ImGuiContextManager()
	{
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}
};

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void Application::InitializeComponents()
{
#if defined(_DEBUG)
	ENABLE_LEAK_DETECTION();
	SET_LEAK_BREAKPOINT(-1);
#endif

	// Initialize ExecutableDirectory
	int		argc;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	if (argv)
	{
		ExecutableDirectory = std::filesystem::path(argv[0]).parent_path();
		LocalFree(argv);
	}

	// Initialize Log
	Log::Create();
}

int Application::Run(Application& Application, const ApplicationOptions& Options)
{
	Microsoft::WRL::Wrappers::RoInitializeWrapper InitializeWinRT(RO_INIT_MULTITHREADED);

	HINSTANCE hInstance = GetModuleHandle(nullptr);

	if (!Options.Icon.empty())
	{
		hIcon = wil::unique_hicon(reinterpret_cast<HICON>(
			::LoadImage(nullptr, Options.Icon.wstring().data(), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_LOADFROMFILE)));
	}

	hCursor = wil::unique_hcursor(::LoadCursor(nullptr, IDC_ARROW));

	// Register window class
	WNDCLASSEXW wcexw	= {};
	wcexw.cbSize		= sizeof(WNDCLASSEX);
	wcexw.style			= CS_HREDRAW | CS_VREDRAW;
	wcexw.lpfnWndProc	= WindowProc;
	wcexw.cbClsExtra	= 0;
	wcexw.cbWndExtra	= 0;
	wcexw.hInstance		= hInstance;
	wcexw.hIcon			= hIcon.get();
	wcexw.hCursor		= hCursor.get();
	wcexw.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
	wcexw.lpszMenuName	= nullptr;
	wcexw.lpszClassName = L"Kai Window Class";
	wcexw.hIconSm		= hIcon.get();
	if (!RegisterClassExW(&wcexw))
	{
		ErrorExit(TEXT("RegisterClassExW"));
	}

	// Create window
	DWORD dwStyle = WS_OVERLAPPEDWINDOW;
	dwStyle |= Options.Maximize ? WS_MAXIMIZE : 0;

	RECT WindowRect = { 0, 0, static_cast<LONG>(Options.Width), static_cast<LONG>(Options.Height) };
	AdjustWindowRect(&WindowRect, dwStyle, FALSE);

	int x		 = Options.x.value_or(CW_USEDEFAULT);
	int y		 = Options.y.value_or(CW_USEDEFAULT);
	WindowWidth	 = WindowRect.right - WindowRect.left;
	WindowHeight = WindowRect.bottom - WindowRect.top;

	hWnd = wil::unique_hwnd(::CreateWindowW(
		wcexw.lpszClassName,
		Options.Name.data(),
		dwStyle,
		x,
		y,
		WindowWidth,
		WindowHeight,
		nullptr, // No parent window
		nullptr, // No menus
		hInstance,
		&Application));
	if (!hWnd)
	{
		ErrorExit(TEXT("CreateWindowW"));
	}

	// Register prt sc hotkey to the window
	RegisterHotKey(hWnd.get(), PRINTSCREEN, 0, VK_SNAPSHOT);

	// Initialize ImGui
	ImGuiContextManager imgui(hWnd.get());

	// Initialize InputHandler
	InputHandler = { hWnd.get() };

	::ShowWindow(hWnd.get(), SW_SHOW);

	Application.Initialize();

	Stopwatch.Restart();
	do
	{
		Stopwatch.Signal();
		Application.Update(static_cast<float>(Stopwatch.DeltaTime()));
	} while (ProcessMessages());

	Application.Shutdown();

	BOOL b = ::UnregisterClass(wcexw.lpszClassName, hInstance);
	return !b;
}

bool Application::ProcessMessages()
{
	MSG Msg = {};
	while (::PeekMessage(&Msg, nullptr, 0, 0, PM_REMOVE))
	{
		::TranslateMessage(&Msg);
		::DispatchMessage(&Msg);

		if (Msg.message == WM_QUIT)
		{
			return false;
		}
	}
	return true;
}

LRESULT CALLBACK Application::WindowProc(_In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
	{
		return true;
	}

	Application* App = reinterpret_cast<Application*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

	InputHandler.Process(uMsg, wParam, lParam);

	switch (uMsg)
	{
	case WM_CREATE:
	{
		// Save the Application* passed in to CreateWindow.
		LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
	}
	break;

	case WM_GETMINMAXINFO: // Catch this message so to prevent the window from becoming too small.
	{
		reinterpret_cast<MINMAXINFO*>(lParam)->ptMinTrackSize.x = GetSystemMetrics(SM_CXMINTRACK);
		reinterpret_cast<MINMAXINFO*>(lParam)->ptMinTrackSize.y = GetSystemMetrics(SM_CYMINTRACK);
	}
	break;

	case WM_SIZE:
	{
		if (wParam == SIZE_MINIMIZED)
		{
			break;
		}

		WindowWidth	 = LOWORD(lParam);
		WindowHeight = HIWORD(lParam);

		if (wParam == SIZE_MINIMIZED)
		{
			Minimized = true;
			Maximized = false;
		}
		else if (wParam == SIZE_MAXIMIZED)
		{
			Minimized = false;
			Maximized = true;
			if (App)
				App->Resize(WindowWidth, WindowHeight);
		}
		else if (wParam == SIZE_RESTORED)
		{
			// Restoring from minimized state?
			if (Minimized)
			{
				Minimized = false;
				if (App)
					App->Resize(WindowWidth, WindowHeight);
			}
			// Restoring from maximized state?
			else if (Maximized)
			{
				Maximized = false;
				if (App)
					App->Resize(WindowWidth, WindowHeight);
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
			else // API call such as SetWindowPos or mSwapChain->SetFullscreenState.
			{
				if (App)
					App->Resize(WindowWidth, WindowHeight);
			}
		}
	}
	break;

	case WM_HOTKEY:
	{
		switch (wParam)
		{
		case PRINTSCREEN:
		{
			// Save back buffer here or something
		}
		break;
		}
	}
	break;

	case WM_ACTIVATE:
	{
		// Confine/free cursor on window to foreground/background if cursor disabled
		if (!InputHandler.CursorEnabled)
		{
			if (wParam & WA_ACTIVE)
			{
				InputHandler.ConfineCursor();
				InputHandler.HideCursor();
			}
			else
			{
				InputHandler.FreeCursor();
				InputHandler.ShowCursor();
			}
		}
	}
	break;

	case WM_DESTROY:
	{
		PostQuitMessage(0);
	}
	break;

	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return 0;
}

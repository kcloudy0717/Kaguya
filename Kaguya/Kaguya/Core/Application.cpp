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
	Log::Initialize();
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
	AspectRatio	 = float(WindowWidth) / float(WindowHeight);

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

	// Initialize InputHandler
	InputHandler = { hWnd.get() };

	::ShowWindow(hWnd.get(), SW_SHOW);

	Initialized = Application.Initialize();

	Stopwatch.Restart();
	do
	{
	} while (ProcessMessages());

	Application.Shutdown();

	BOOL b = ::UnregisterClass(wcexw.lpszClassName, hInstance);
	return !b;
}

std::filesystem::path Application::OpenDialog(UINT NumFilters, COMDLG_FILTERSPEC* pFilterSpecs)
{
	// COMDLG_FILTERSPEC ComDlgFS[3] = { { L"C++ code files", L"*.cpp;*.h;*.rc" },
	//								  { L"Executable Files", L"*.exe;*.dll" },
	//								  { L"All Files (*.*)", L"*.*" } };
	using Microsoft::WRL::ComPtr;

	std::filesystem::path	Path;
	ComPtr<IFileOpenDialog> pFileOpen;

	if (SUCCEEDED(CoCreateInstance(
			CLSID_FileOpenDialog,
			nullptr,
			CLSCTX_ALL,
			IID_PPV_ARGS(pFileOpen.ReleaseAndGetAddressOf()))))
	{
		pFileOpen->SetFileTypes(NumFilters, pFilterSpecs);

		// Show the Open dialog box.
		// Get the file name from the dialog box.
		if (SUCCEEDED(pFileOpen->Show(nullptr)))
		{
			ComPtr<IShellItem> pItem;
			if (SUCCEEDED(pFileOpen->GetResult(&pItem)))
			{
				PWSTR pszFilePath;
				// Display the file name to the user.
				if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath)))
				{
					Path = pszFilePath;
					CoTaskMemFree(pszFilePath);
				}
			}
		}
	}

	return Path;
}

std::filesystem::path Application::SaveDialog(UINT NumFilters, COMDLG_FILTERSPEC* pFilterSpecs)
{
	using Microsoft::WRL::ComPtr;

	std::filesystem::path	Path;
	ComPtr<IFileSaveDialog> pFileSave;

	if (SUCCEEDED(CoCreateInstance(
			CLSID_FileSaveDialog,
			nullptr,
			CLSCTX_ALL,
			IID_PPV_ARGS(pFileSave.ReleaseAndGetAddressOf()))))
	{
		pFileSave->SetFileTypes(NumFilters, pFilterSpecs);

		// Show the Save dialog box.
		// Get the file name from the dialog box.
		if (SUCCEEDED(pFileSave->Show(nullptr)))
		{
			ComPtr<IShellItem> pItem;
			if (SUCCEEDED(pFileSave->GetResult(&pItem)))
			{
				PWSTR pszFilePath;
				// Display the file name to the user.
				if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath)))
				{
					Path = pszFilePath;
					CoTaskMemFree(pszFilePath);
				}
			}
		}
	}

	return Path;
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

	case WM_PAINT:
	{
		Stopwatch.Signal();
		App->Update(static_cast<float>(Stopwatch.GetDeltaTime()));
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
		AspectRatio	 = float(WindowWidth) / float(WindowHeight);

		if (wParam == SIZE_MINIMIZED)
		{
			Minimized = true;
			Maximized = false;
		}
		else if (wParam == SIZE_MAXIMIZED)
		{
			Minimized = false;
			Maximized = true;
			if (App && Initialized)
			{
				App->Resize(WindowWidth, WindowHeight);
			}
		}
		else if (wParam == SIZE_RESTORED)
		{
			// Restoring from minimized state?
			if (Minimized)
			{
				Minimized = false;
				if (App && Initialized)
				{
					App->Resize(WindowWidth, WindowHeight);
				}
			}
			// Restoring from maximized state?
			else if (Maximized)
			{
				Maximized = false;
				if (App && Initialized)
				{
					App->Resize(WindowWidth, WindowHeight);
				}
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
				if (App && Initialized)
				{
					App->Resize(WindowWidth, WindowHeight);
				}
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

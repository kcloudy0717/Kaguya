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

using Microsoft::WRL::ComPtr;

// https://www.gamedev.net/forums/topic/693260-vk_snapshot-and-keydown/
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

Application::Application(const std::string& LoggerName)
{
#if defined(_DEBUG)
	ENABLE_LEAK_DETECTION();
	SET_LEAK_BREAKPOINT(-1);
#endif

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
}

int Application::Run(Application& Application, const ApplicationOptions& Options)
{
	Microsoft::WRL::Wrappers::RoInitializeWrapper InitializeWinRT(RO_INIT_MULTITHREADED);

	HINSTANCE hInstance = GetModuleHandle(nullptr);

	if (!Options.Icon.empty())
	{
		assert(Options.Icon.extension() == ".ico");
		hIcon = wil::unique_hicon(static_cast<HICON>(
			LoadImage(nullptr, Options.Icon.wstring().data(), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_LOADFROMFILE)));
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
	wcexw.lpszClassName = TEXT("Kai Window Class");
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
	AspectRatio	 = static_cast<float>(WindowWidth) / static_cast<float>(WindowHeight);

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
	ImGuiContextManager ImGuiContextManager(hWnd.get());

	// Initialize InputHandler
	InputHandler = { hWnd.get() };

	ShowWindow(hWnd.get(), SW_SHOW);

	Initialized = Application.Initialize();

	Stopwatch.Restart();
	do
	{
		Stopwatch.Signal();
		Application.Update(static_cast<float>(Stopwatch.GetDeltaTime()));
	} while (ProcessMessages());

	Application.Shutdown();

	UnregisterClass(wcexw.lpszClassName, hInstance);
	return ExitCode;
}

std::filesystem::path Application::OpenDialog(UINT NumFilters, const COMDLG_FILTERSPEC* FilterSpecs)
{
	// COMDLG_FILTERSPEC ComDlgFS[3] = { { L"C++ code files", L"*.cpp;*.h;*.rc" },
	//								  { L"Executable Files", L"*.exe;*.dll" },
	//								  { L"All Files (*.*)", L"*.*" } };
	std::filesystem::path	Path;
	ComPtr<IFileOpenDialog> FileOpen;
	if (SUCCEEDED(CoCreateInstance(
			CLSID_FileOpenDialog,
			nullptr,
			CLSCTX_ALL,
			IID_PPV_ARGS(FileOpen.ReleaseAndGetAddressOf()))))
	{
		FileOpen->SetFileTypes(NumFilters, FilterSpecs);

		// Show the Open dialog box.
		// Get the file name from the dialog box.
		if (SUCCEEDED(FileOpen->Show(nullptr)))
		{
			ComPtr<IShellItem> Item;
			if (SUCCEEDED(FileOpen->GetResult(&Item)))
			{
				PWSTR pszFilePath;
				// Display the file name to the user.
				if (SUCCEEDED(Item->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath)))
				{
					Path = pszFilePath;
					CoTaskMemFree(pszFilePath);
				}
			}
		}
	}

	return Path;
}

std::filesystem::path Application::SaveDialog(UINT NumFilters, const COMDLG_FILTERSPEC* FilterSpecs)
{
	std::filesystem::path	Path;
	ComPtr<IFileSaveDialog> FileSave;
	if (SUCCEEDED(CoCreateInstance(
			CLSID_FileSaveDialog,
			nullptr,
			CLSCTX_ALL,
			IID_PPV_ARGS(FileSave.ReleaseAndGetAddressOf()))))
	{
		FileSave->SetFileTypes(NumFilters, FilterSpecs);

		// Show the Save dialog box.
		// Get the file name from the dialog box.
		if (SUCCEEDED(FileSave->Show(nullptr)))
		{
			ComPtr<IShellItem> Item;
			if (SUCCEEDED(FileSave->GetResult(&Item)))
			{
				PWSTR pszFilePath;
				// Display the file name to the user.
				if (SUCCEEDED(Item->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath)))
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
	while (PeekMessage(&Msg, nullptr, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);

		if (Msg.message == WM_QUIT)
		{
			ExitCode = static_cast<int>(Msg.wParam);
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

	auto This = std::bit_cast<Application*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

	InputHandler.Process(uMsg, wParam, lParam);

	switch (uMsg)
	{
	case WM_CREATE:
	{
		// Save the Application* passed in to CreateWindow.
		auto CreateStruct = std::bit_cast<LPCREATESTRUCT>(lParam);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(CreateStruct->lpCreateParams));
	}
	break;

	case WM_GETMINMAXINFO: // Catch this message so to prevent the window from becoming too small.
	{
		auto Info			 = std::bit_cast<MINMAXINFO*>(lParam);
		Info->ptMinTrackSize = { GetSystemMetrics(SM_CXMINTRACK), GetSystemMetrics(SM_CYMINTRACK) };
	}
	break;

		// case WM_PAINT:
		//{
		//	Stopwatch.Signal();
		//	This->Update(static_cast<float>(Stopwatch.GetDeltaTime()));
		//}
		// break;

	case WM_SIZE:
	{
		if (wParam == SIZE_MINIMIZED)
		{
			break;
		}

		WindowWidth	 = LOWORD(lParam);
		WindowHeight = HIWORD(lParam);
		AspectRatio	 = static_cast<float>(WindowWidth) / static_cast<float>(WindowHeight);

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
			if (This && Initialized)
			{
				This->Resize(WindowWidth, WindowHeight);
			}
		}
	}
	break;

	case WM_HOTKEY:
	{
		if (wParam == PRINTSCREEN)
		{
			UNREFERENCED_PARAMETER(wParam);
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

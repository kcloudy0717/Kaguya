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
		std::filesystem::path executablePath{ argv[0] };
		ExecutableDirectory = executablePath.parent_path();
		LocalFree(argv);
	}

	// Initialize Log
	Log::Create();
}

void Application::Initialize(const ApplicationOptions& Options)
{
	int x = Options.x.value_or(CW_USEDEFAULT);
	int y = Options.y.value_or(CW_USEDEFAULT);

	Window.SetIcon(::LoadImage(0, Options.Icon.wstring().data(), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_LOADFROMFILE));

	Window.Create(Options.Name.data(), Options.Width, Options.Height, x, y, Options.Maximize);

	// Initialize input handler
	m_InputHandler = InputHandler(Window.GetWindowHandle());
}

int Application::Run()
{
	Microsoft::WRL::Wrappers::RoInitializeWrapper InitializeWinRT(RO_INIT_MULTITHREADED);

	// Begin our render thread
	m_RenderThread = wil::unique_handle(::CreateThread(nullptr, 0, Application::RenderThreadProc, nullptr, 0, nullptr));

	MSG msg = {};
	while (msg.message != WM_QUIT)
	{
		if (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);

			m_InputHandler.Handle(&msg);
		}
	}

	// Set m_ExitRenderThread to true and wait for it to join
	if (m_RenderThread)
	{
		m_ExitRenderThread = true;
		::WaitForSingleObject(m_RenderThread.get(), INFINITE);
	}

	return (int)msg.wParam;
}

DWORD WINAPI Application::RenderThreadProc(_In_ PVOID pParameter)
{
	SetThreadDescription(GetCurrentThread(), L"Render Thread");

	Microsoft::WRL::Wrappers::RoInitializeWrapper InitializeWinRT(RO_INIT_MULTITHREADED);

	while (true)
	{
		if (m_ExitRenderThread)
		{
			break;
		}

		// Process window messages
		Window::Message messsage = {};
		Window::Message resizeMessage(Window::Message::EType::None, {});
		while (Window.MessageQueue.pop(messsage, 0))
		{
			if (messsage.Type == Window::Message::EType::Resize)
			{
				resizeMessage = messsage;
				continue;
			}

			HandleRenderMessage(messsage);
		}

		// Now we process resize message
		if (resizeMessage.Type == Window::Message::EType::Resize)
		{
			if (!HandleRenderMessage(resizeMessage))
			{
				// Requeue this message if it failed to resize
				Window.MessageQueue.push(resizeMessage);
			}
		}

		Window.Render();
	}

	return EXIT_SUCCESS;
}

bool Application::HandleRenderMessage(const Window::Message& Message)
{
	switch (Message.Type)
	{
	case Window::Message::EType::Resize:
	{
		Window.Resize(Message.Data.Width, Message.Data.Height);
		return true;
	}
	break;
	}

	return false;
}

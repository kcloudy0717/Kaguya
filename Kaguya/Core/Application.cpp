#include "pch.h"
#include "Application.h"

#include <shellapi.h>

#pragma comment(lib, "runtimeobject.lib") 

void Application::InitializeComponents()
{
	int argc;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	if (argv)
	{
		std::filesystem::path executablePath{ argv[0] };
		ExecutableDirectory = executablePath.parent_path();
		LocalFree(argv);
	}
}

void Application::Initialize(const ApplicationOptions& Options)
{
	Log::Create();

	auto icoFile = Application::ExecutableDirectory / "Assets/Kaguya.ico";
	Window.SetIcon(::LoadImage(0, icoFile.wstring().data(), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_LOADFROMFILE));
	Window.Create(Options.Title.data(), Options.Width, Options.Height, Options.X, Options.Y, Options.Maximize);
	
	// Initialize input handler
	m_InputHandler = InputHandler(Window.GetWindowHandle());
}

int Application::Run(std::function<void()> ShutdownFunc)
{
	Microsoft::WRL::Wrappers::RoInitializeWrapper InitializeWinRT(RO_INIT_MULTITHREADED);

	// Begin our render thread
	RenderThread = wil::unique_handle(::CreateThread(nullptr, 0, Application::RenderThreadProc, nullptr, 0, nullptr));
	if (!RenderThread)
	{
		LOG_ERROR("Failed to create thread (error={})", ::GetLastError());
	}

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

	// Set ExitRenderThread to true and wait for it to join
	if (RenderThread)
	{
		ExitRenderThread = true;
		::WaitForSingleObject(RenderThread.get(), INFINITE);
	}

	if (ShutdownFunc)
	{
		ShutdownFunc();
	}

	int exitCode = (int)msg.wParam;
	LOG_INFO("Exit Code: {}", exitCode);
	return exitCode;
}

DWORD WINAPI Application::RenderThreadProc(_In_ PVOID pParameter)
{
	SetThreadDescription(GetCurrentThread(), L"Render Thread");

	Microsoft::WRL::Wrappers::RoInitializeWrapper InitializeWinRT(RO_INIT_MULTITHREADED);

	while (true)
	{
		if (ExitRenderThread)
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

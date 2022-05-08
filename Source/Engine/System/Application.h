#pragma once
#include <filesystem>
#include "Window.h"
#include "InputManager.h"
#include "Delegate.h"

class IApplicationMessageHandler;

class Application
{
public:
	using MessageCallback = void (*)(void*, HWND, UINT, WPARAM, LPARAM);

	Application();
	virtual ~Application();

	void Run();

	void SetMessageHandler(IApplicationMessageHandler* MessageHandler);

	void AddWindow(Window* Parent, Window* Window, const WINDOW_DESC& Desc);

	void SetRawInputMode(bool Enable, Window* Window);

	virtual bool Initialize() { return true; }
	virtual void Shutdown() {}
	virtual void Update() {}

private:
	static LRESULT CALLBACK WindowProc(_In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam);

	LRESULT ProcessMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
	inline static InputManager InputManager;

private:
	struct PlatformWindows
	{
		PlatformWindows()
			: Result(CoInitializeEx(nullptr, COINIT_MULTITHREADED))
		{
		}
		~PlatformWindows()
		{
			if (SUCCEEDED(Result))
			{
				CoUninitialize();
			}
		}
		HRESULT Result = S_FALSE;
	} PlatformWindows;

	HINSTANCE HInstance;

protected:
	bool Minimized = false;
	bool Maximized = false;
	bool Resizing  = false;

	bool RequestExit = false;

private:
	bool Initialized = false;

	IApplicationMessageHandler* MessageHandler = nullptr;
	std::vector<Window*>		Windows;

public:
	MulticastDelegate<HWND /*HWnd*/, UINT /*Message*/, WPARAM /*WParam*/, LPARAM /*LParam*/> Callbacks;
};

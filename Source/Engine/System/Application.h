#pragma once
#include "Window.h"
#include "InputManager.h"
#include "Delegate.h"

class IApplicationMessageHandler;

class Application
{
public:
	Application();
	virtual ~Application();

	void Run();

	void SetMessageHandler(IApplicationMessageHandler* MessageHandler);

	void AddWindow(Window* Parent, Window* Window, const WindowDesc& Desc);

	void SetRawInputMode(bool Enable, Window* Window);

	virtual bool Initialize() { return true; }
	virtual void Shutdown() {}
	virtual void Update() {}

private:
	static LRESULT WINDOWS_CALLING_CONVENTION WindowProc(HWND HWnd, u32 Message, WPARAM WParam, LPARAM LParam);
	LRESULT									  ProcessMessage(HWND HWnd, u32 Message, WPARAM WParam, LPARAM LParam);

public:
	inline static InputManager InputManager;

private:
	struct PlatformWindows
	{
		PlatformWindows();
		~PlatformWindows();

		bool Initialized = false;
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
	MulticastDelegate<HWND /*HWnd*/, u32 /*Message*/, WPARAM /*WParam*/, LPARAM /*LParam*/> Callbacks;
};

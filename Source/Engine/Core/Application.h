#pragma once
#include <filesystem>

#include "Window.h"
#include "Stopwatch.h"
#include "InputManager.h"

struct ApplicationOptions
{
	std::filesystem::path Icon;
};

class IApplicationMessageHandler;

class Application
{
public:
	inline static LPCWSTR WindowClass = L"KaguyaWindow";

	explicit Application(const std::string& LoggerName, const ApplicationOptions& Options);
	virtual ~Application();

	void Run();

	void SetMessageHandler(IApplicationMessageHandler* MessageHandler);

	void AddWindow(Window* Parent, Window* Window, const WINDOW_DESC& Desc);

	void SetRawInputMode(bool Enable, Window* Window);

	virtual bool Initialize()			 = 0;
	virtual void Shutdown()				 = 0;
	virtual void Update(float DeltaTime) = 0;

private:
	static LRESULT CALLBACK WindowProc(_In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam);

	void	PumpMessages();
	LRESULT ProcessMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
	// Components
	inline static std::filesystem::path ExecutableDirectory;
	inline static InputManager			InputManager;

private:
	Microsoft::WRL::Wrappers::RoInitializeWrapper InitializeWrapper;
	HINSTANCE									  HInstance;

protected:
	wil::unique_hicon	HIcon;
	wil::unique_hcursor HCursor;
	Stopwatch			Stopwatch;

	bool Minimized = false;
	bool Maximized = false;
	bool Resizing  = false;

	bool RequestExit = false;

	float DeltaTime;

private:
	bool Initialized = false;

	IApplicationMessageHandler* MessageHandler = nullptr;
	std::vector<Window*>		Windows;
};

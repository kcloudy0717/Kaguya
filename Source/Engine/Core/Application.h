#pragma once
#include <filesystem>

#include "Stopwatch.h"
#include "InputHandler.h"

struct ApplicationOptions
{
	std::wstring		  Name;
	int					  Width	 = CW_USEDEFAULT;
	int					  Height = CW_USEDEFAULT;
	std::optional<int>	  x;
	std::optional<int>	  y;
	bool				  Maximize = false;
	std::filesystem::path Icon;
};

class Application final
{
public:
	explicit Application(const std::string& LoggerName);

	int Run(const ApplicationOptions& Options);

	static int	 GetWidth() { return WindowWidth; }
	static int	 GetHeight() { return WindowHeight; }
	static float GetAspectRatio() { return AspectRatio; }
	static HWND	 GetWindowHandle() { return hWnd.get(); }

	static InputHandler& GetInputHandler() { return InputHandler; }
	static Mouse&		 GetMouse() { return InputHandler.Mouse; }
	static Keyboard&	 GetKeyboard() { return InputHandler.Keyboard; }

	static std::filesystem::path OpenDialog(std::span<COMDLG_FILTERSPEC> FilterSpecs);
	static std::filesystem::path SaveDialog(std::span<COMDLG_FILTERSPEC> FilterSpecs);

	Delegate<bool()>						Initialize;
	Delegate<void()>						Shutdown;
	Delegate<void(float DeltaTime)>			Update;
	Delegate<void(UINT Width, UINT Height)> Resize;

private:
	static bool ProcessMessages();

	static LRESULT CALLBACK WindowProc(_In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam);

public:
	// Components
	inline static std::filesystem::path ExecutableDirectory;

protected:
	inline static UINT				  WindowWidth, WindowHeight;
	inline static float				  AspectRatio;
	inline static wil::unique_hicon	  hIcon;
	inline static wil::unique_hcursor hCursor;
	inline static wil::unique_hwnd	  hWnd;
	inline static InputHandler		  InputHandler;
	inline static Stopwatch			  Stopwatch;

	inline static bool Minimized = false;
	inline static bool Maximized = false;
	inline static bool Resizing	 = false;

private:
	inline static bool Initialized = false;
	inline static int  ExitCode	   = 0;
};

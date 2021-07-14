#pragma once
#include <filesystem>
#include <type_traits>
#include <string_view>

#include "Stopwatch.h"
#include "InputHandler.h"

struct ApplicationOptions
{
	std::wstring_view	  Name;
	int					  Width	 = CW_USEDEFAULT;
	int					  Height = CW_USEDEFAULT;
	std::optional<int>	  x;
	std::optional<int>	  y;
	bool				  Maximize = false;
	std::filesystem::path Icon;
};

class Application
{
public:
	static void InitializeComponents();

	static int Run(Application& Application, const ApplicationOptions& Options);

	static int	 GetWidth() { return WindowWidth; }
	static int	 GetHeight() { return WindowHeight; }
	static float GetAspectRatio() { return AspectRatio; }
	static HWND	 GetWindowHandle() { return hWnd.get(); }

	static InputHandler& GetInputHandler() { return InputHandler; }
	static Mouse&		 GetMouse() { return InputHandler.Mouse; }
	static Keyboard&	 GetKeyboard() { return InputHandler.Keyboard; }

	static std::filesystem::path OpenDialog(UINT NumFilters, COMDLG_FILTERSPEC* pFilterSpecs);
	static std::filesystem::path SaveDialog(UINT NumFilters, COMDLG_FILTERSPEC* pFilterSpecs);

	virtual bool Initialize()					 = 0;
	virtual void Update(float DeltaTime)		 = 0;
	virtual void Shutdown()						 = 0;
	virtual void Resize(UINT Width, UINT Height) = 0;

private:
	static bool ProcessMessages();

	static LRESULT CALLBACK WindowProc(_In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam);

public:
	// Components
	inline static std::filesystem::path ExecutableDirectory;

protected:
	inline static int				  WindowWidth, WindowHeight;
	inline static float				  AspectRatio;
	inline static wil::unique_hicon	  hIcon;
	inline static wil::unique_hcursor hCursor;
	inline static wil::unique_hwnd	  hWnd;
	inline static InputHandler		  InputHandler;
	inline static Stopwatch			  Stopwatch;

	inline static bool Minimized = false; // is the application minimized?
	inline static bool Maximized = false; // is the application maximized?
	inline static bool Resizing	 = false; // are the resize bars being dragged?

private:
	inline static bool Initialized = false;
};

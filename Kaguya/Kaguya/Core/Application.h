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

template<class T>
concept IsDialogFunction = requires(T F, std::filesystem::path Path)
{
	{
		F(Path)
		} -> std::convertible_to<void>;
};

template<IsDialogFunction DialogFunction>
inline void OpenDialog(std::string_view FilterList, std::string_view DefaultPath, DialogFunction Function)
{
	nfdchar_t*	outPath = nullptr;
	nfdresult_t result =
		NFD_OpenDialog(FilterList.data(), DefaultPath.empty() ? DefaultPath.data() : nullptr, &outPath);

	if (result == NFD_OKAY)
	{
		Function(std::filesystem::path(outPath));
		free(outPath);
	}
	else if (result == NFD_CANCEL)
	{
		UNREFERENCED_PARAMETER(result);
	}
	else
	{
		printf("Error: %s\n", NFD_GetError());
	}
}

// Function is called for each path
template<IsDialogFunction DialogFunction>
inline void OpenDialogMultiple(std::string_view FilterList, std::string_view DefaultPath, DialogFunction Function)
{
	nfdpathset_t pathSet;
	nfdresult_t	 result =
		NFD_OpenDialogMultiple(FilterList.data(), DefaultPath.empty() ? DefaultPath.data() : nullptr, &pathSet);
	if (result == NFD_OKAY)
	{
		size_t i;
		for (i = 0; i < NFD_PathSet_GetCount(&pathSet); ++i)
		{
			nfdchar_t* path = NFD_PathSet_GetPath(&pathSet, i);
			Function(std::filesystem::path(path));
		}
		NFD_PathSet_Free(&pathSet);
	}
	else if (result == NFD_CANCEL)
	{
		UNREFERENCED_PARAMETER(result);
	}
	else
	{
		printf("Error: %s\n", NFD_GetError());
	}
}

template<IsDialogFunction DialogFunction>
inline void SaveDialog(std::string_view FilterList, std::string_view DefaultPath, DialogFunction Function)
{
	nfdchar_t*	savePath = nullptr;
	nfdresult_t result =
		NFD_SaveDialog(FilterList.data(), DefaultPath.empty() ? DefaultPath.data() : nullptr, &savePath);
	if (result == NFD_OKAY)
	{
		Function(std::filesystem::path(savePath));
		free(savePath);
	}
	else if (result == NFD_CANCEL)
	{
		UNREFERENCED_PARAMETER(result);
	}
	else
	{
		printf("Error: %s\n", NFD_GetError());
	}
}

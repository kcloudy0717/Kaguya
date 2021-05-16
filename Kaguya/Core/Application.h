#pragma once
#include <filesystem>
#include <type_traits>
#include <string_view>

#include "Stopwatch.h"
#include "Window.h"
#include "InputHandler.h"

struct ApplicationOptions
{
	std::wstring Title = L"Application";
	int Width = CW_USEDEFAULT;
	int Height = CW_USEDEFAULT;
	int X = CW_USEDEFAULT;
	int Y = CW_USEDEFAULT;
	bool Maximize = true;
};

class Application
{
public:
	static void InitializeComponents();

	static void Initialize(const ApplicationOptions& Options);

	static int Run();

	static int Width() { return Window.GetWindowWidth(); }
	static int Height() { return Window.GetWindowHeight(); }
	static float AspectRatio() { return (float)Width() / (float)Height(); }

	static InputHandler& GetInputHandler() { return m_InputHandler; }
	static Mouse& GetMouse() { return m_InputHandler.Mouse; }
	static Keyboard& GetKeyboard() { return m_InputHandler.Keyboard; }

private:
	static DWORD WINAPI RenderThreadProc(_In_ PVOID pParameter);
	static bool HandleRenderMessage(const Window::Message& Message);

public:
	inline static std::filesystem::path ExecutableDirectory;
	inline static Stopwatch Time;
	inline static Window Window;

private:
	inline static wil::unique_handle RenderThread;
	inline static std::atomic<bool> ExitRenderThread = false;
	inline static InputHandler m_InputHandler;
};

template<class T>
concept IsDialogFunction = requires(T F, std::filesystem::path Path)
{
	{ F(Path) } -> std::convertible_to<void>;
};

template<IsDialogFunction DialogFunction>
inline void OpenDialog(std::string_view FilterList, std::string_view DefaultPath, DialogFunction Function)
{
	nfdchar_t* outPath = nullptr;
	nfdresult_t result = NFD_OpenDialog(FilterList.data(), DefaultPath.empty() ? DefaultPath.data() : nullptr, &outPath);

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
	nfdresult_t result = NFD_OpenDialogMultiple(FilterList.data(), DefaultPath.empty() ? DefaultPath.data() : nullptr, &pathSet);
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
	nfdchar_t* savePath = nullptr;
	nfdresult_t result = NFD_SaveDialog(FilterList.data(), DefaultPath.empty() ? DefaultPath.data() : nullptr, &savePath);
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
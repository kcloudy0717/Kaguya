#pragma once
#include <Windows.h>
#include <wil/resource.h>
#include <string>

#include "ThreadSafeQueue.h"

class Window
{
public:
	using ResizeFunc = std::function<void(UINT Width, UINT Height)>;

	struct Message
	{
		enum class EType
		{
			None,
			Resize
		};

		struct SData
		{
			UINT Width, Height;
		};

		Message() = default;
		Message(EType Type, SData Data)
			: Type(Type),
			Data(Data)
		{

		}

		EType Type;
		SData Data;
	};

	Window() noexcept = default;
	~Window();

	void SetIcon(HANDLE Image);
	void SetCursor(HCURSOR Cursor);

	void Create(LPCWSTR WindowName,
		int Width = CW_USEDEFAULT, int Height = CW_USEDEFAULT,
		int X = CW_USEDEFAULT, int Y = CW_USEDEFAULT, bool Maximize = false);

	void EnableCursor();
	void DisableCursor();
	bool CursorEnabled() const;
	bool IsFocused() const;

	void Render();
	void Resize(UINT Width, UINT Height);

	inline auto GetWindowHandle() const { return m_WindowHandle.get(); }
	inline auto GetWindowName() const { return m_WindowName; }
	inline auto GetWindowWidth() const { return m_WindowWidth; }
	inline auto GetWindowHeight() const { return m_WindowHeight; }

	void SetRenderFunc(std::function<void()> RenderFunc);
	void SetResizeFunc(ResizeFunc ResizeFunc);

private:
	LRESULT DispatchEvent(UINT uMsg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void ConfineCursor();
	void FreeCursor();
	void ShowCursor();
	void HideCursor();

	class ImGuiContextManager
	{
	public:
		ImGuiContextManager();
		~ImGuiContextManager();
	};

public:
	// Right now this queue is exclusively available to the render thread (it can push and pop) need to
	// come up with something better in case in the future it might be used by mutiple threads
	ThreadSafeQueue<Message> MessageQueue;

private:
	std::wstring m_WindowName;
	unsigned int m_WindowWidth = 0;
	unsigned int m_WindowHeight = 0;
	bool m_CursorEnabled = true;

	wil::unique_hicon m_Icon;
	wil::unique_hcursor m_Cursor;
	wil::unique_hwnd m_WindowHandle;

	ImGuiContextManager m_ImGuiContextManager;

	std::function<void()> m_RenderFunc;
	ResizeFunc m_ResizeFunc;
};
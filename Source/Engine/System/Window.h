#pragma once
#include "Types.h"
#include "Platform.h"

class Application;

enum class WindowInitialSize
{
	Default,
	Maximize
};

struct WindowDesc
{
	static constexpr i32 SystemDefault = ~0;

	std::wstring_view Name;
	i32				  Width		  = SystemDefault;
	i32				  Height	  = SystemDefault;
	i32				  x			  = SystemDefault;
	i32				  y			  = SystemDefault;
	WindowInitialSize InitialSize = WindowInitialSize::Default;
};

class Window
{
public:
	static constexpr std::wstring_view WindowClass = L"KaguyaWindow";

	[[nodiscard]] const WindowDesc& GetDesc() const noexcept;
	[[nodiscard]] HWND				GetWindowHandle() const noexcept;
	[[nodiscard]] i32				GetWidth() const noexcept;
	[[nodiscard]] i32				GetHeight() const noexcept;

	void Show();
	void Hide();
	void Destroy();

	void SetRawInput(bool Enable);

	[[nodiscard]] bool IsMaximized() const noexcept;
	[[nodiscard]] bool IsMinimized() const noexcept;
	[[nodiscard]] bool IsVisible() const noexcept;
	[[nodiscard]] bool IsForeground() const noexcept;
	[[nodiscard]] bool IsUsingRawInput() const noexcept;
	[[nodiscard]] bool IsPointInWindow(i32 X, i32 Y) const noexcept;

	void Resize(i32 Width, i32 Height);

private:
	friend class Application;

	void Initialize(Application* Application, Window* Parent, HINSTANCE HInstance, const WindowDesc& Desc);

	Application* OwningApplication = nullptr;
	HINSTANCE	 HInstance		   = nullptr;

	WindowDesc		   Desc = {};
	ScopedWindowHandle WindowHandle;

	i32 WindowWidth	 = 0;
	i32 WindowHeight = 0;

	bool Visible  = false;
	bool RawInput = false;
};

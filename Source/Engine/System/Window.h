#pragma once
#include <wil/resource.h>

class Application;

enum class WindowInitialSize
{
	Default,
	Maximize
};

struct WINDOW_DESC
{
	LPCWSTR			  Name		  = nullptr;
	int				  Width		  = CW_USEDEFAULT;
	int				  Height	  = CW_USEDEFAULT;
	int				  x			  = CW_USEDEFAULT;
	int				  y			  = CW_USEDEFAULT;
	WindowInitialSize InitialSize = WindowInitialSize::Default;
};

class Window
{
public:
	static LPCWSTR WindowClass;

	void Initialize(Application* Application, Window* Parent, HINSTANCE HInstance, const WINDOW_DESC& Desc);

	[[nodiscard]] const WINDOW_DESC& GetDesc() const noexcept;
	[[nodiscard]] HWND				 GetWindowHandle() const noexcept;
	[[nodiscard]] std::int32_t		 GetWidth() const noexcept;
	[[nodiscard]] std::int32_t		 GetHeight() const noexcept;

	void Show();
	void Hide();
	void Destroy();

	void SetRawInput(bool Enable);

	[[nodiscard]] bool IsMaximized() const noexcept;
	[[nodiscard]] bool IsMinimized() const noexcept;
	[[nodiscard]] bool IsVisible() const noexcept;
	[[nodiscard]] bool IsForeground() const noexcept;
	[[nodiscard]] bool IsUsingRawInput() const noexcept;
	[[nodiscard]] bool IsPointInWindow(std::int32_t X, std::int32_t Y) const noexcept;

	void Resize(int Width, int Height);

private:
	Application* OwningApplication = nullptr;
	HINSTANCE	 HInstance		   = nullptr;

	WINDOW_DESC		 Desc = {};
	wil::unique_hwnd WindowHandle;

	std::int32_t WindowWidth  = 0;
	std::int32_t WindowHeight = 0;
	float		 AspectRatio  = 1.0f;

	bool Visible  = false;
	bool RawInput = false;
};

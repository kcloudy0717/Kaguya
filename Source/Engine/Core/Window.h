#pragma once

class Application;

struct WINDOW_DESC
{
	std::wstring	   Name;
	int				   Width  = CW_USEDEFAULT;
	int				   Height = CW_USEDEFAULT;
	std::optional<int> x;
	std::optional<int> y;

	// These values are mutually exclusive
	// Should this window be maximized initially
	bool Maximize = false;
	// Should this window be minimized initially
	bool Minimize = false;
};

class Window
{
public:
	void Initialize(Application* Application, Window* Parent, HINSTANCE HInstance, const WINDOW_DESC& Desc);

	[[nodiscard]] const WINDOW_DESC& GetDesc() const noexcept;
	[[nodiscard]] HWND				 GetWindowHandle() const noexcept;

	void Show();
	void Hide();
	void Destroy();

	void SetRawInput(bool Enable) { RawInput = Enable; }

	[[nodiscard]] bool IsMaximized() const noexcept;
	[[nodiscard]] bool IsMinimized() const noexcept;
	[[nodiscard]] bool IsVisible() const noexcept;
	[[nodiscard]] bool IsForeground() const noexcept;
	[[nodiscard]] bool IsUsingRawInput() const noexcept;

	void Resize(int Width, int Height);

private:
	Application* OwningApplication = nullptr;
	HINSTANCE	 HInstance		   = nullptr;

	WINDOW_DESC		 Desc = {};
	wil::unique_hwnd WindowHandle;

	int	  WindowWidth  = 0;
	int	  WindowHeight = 0;
	float AspectRatio  = 1.0f;

	bool Visible  = false;
	bool RawInput = false;
};

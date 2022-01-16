#include "Window.h"
#include "Application.h"
#include <hidusage.h>

LPCWSTR Window::WindowClass = L"KaguyaWindow";

void Window::Initialize(Application* Application, Window* Parent, HINSTANCE HInstance, const WINDOW_DESC& Desc)
{
	this->OwningApplication = Application;
	this->HInstance			= HInstance;
	this->Desc				= Desc;

	// Create window
	DWORD	ExStyle	   = 0;
	LPCWSTR WindowName = Desc.Name;
	DWORD	Style	   = WS_OVERLAPPEDWINDOW;
	Style |= Desc.InitialSize == WindowInitialSize::Maximize ? WS_MAXIMIZE : 0;

	RECT WindowRect = { 0, 0, static_cast<LONG>(Desc.Width), static_cast<LONG>(Desc.Height) };
	AdjustWindowRect(&WindowRect, Style, FALSE);

	WindowWidth	 = WindowRect.right - WindowRect.left;
	WindowHeight = WindowRect.bottom - WindowRect.top;
	AspectRatio	 = static_cast<float>(WindowWidth) / static_cast<float>(WindowHeight);

	WindowHandle = wil::unique_hwnd(CreateWindowExW(
		ExStyle,
		WindowClass,
		WindowName,
		Style,
		Desc.x,
		Desc.y,
		WindowWidth,
		WindowHeight,
		Parent ? Parent->GetWindowHandle() : nullptr,
		nullptr, // No menus
		HInstance,
		Application));
	if (!WindowHandle)
	{
		ErrorExit(L"CreateWindowExW");
	}

	RECT ClientRect = {};
	::GetClientRect(WindowHandle.get(), &ClientRect);
	Resize(ClientRect.right - ClientRect.left, ClientRect.bottom - ClientRect.top);

	RAWINPUTDEVICE RawInputDevice = {};
	RawInputDevice.usUsagePage	  = HID_USAGE_PAGE_GENERIC;
	RawInputDevice.usUsage		  = HID_USAGE_GENERIC_MOUSE;
	RawInputDevice.dwFlags		  = RIDEV_INPUTSINK;
	RawInputDevice.hwndTarget	  = WindowHandle.get();
	if (!RegisterRawInputDevices(&RawInputDevice, 1, sizeof(RAWINPUTDEVICE)))
	{
		ErrorExit(L"RegisterRawInputDevices");
	}
}

const WINDOW_DESC& Window::GetDesc() const noexcept
{
	return Desc;
}

HWND Window::GetWindowHandle() const noexcept
{
	return WindowHandle.get();
}

std::int32_t Window::GetWidth() const noexcept
{
	return WindowWidth;
}

std::int32_t Window::GetHeight() const noexcept
{
	return WindowHeight;
}

void Window::Show()
{
	if (!Visible)
	{
		Visible = true;

		ShowWindow(WindowHandle.get(), SW_SHOW);
	}
}

void Window::Hide()
{
	if (Visible)
	{
		Visible = false;
		ShowWindow(WindowHandle.get(), SW_HIDE);
	}
}

void Window::Destroy()
{
	WindowHandle.reset();
}

void Window::SetRawInput(bool Enable)
{
	RawInput = Enable;
}

bool Window::IsMaximized() const noexcept
{
	return IsZoomed(WindowHandle.get());
}

bool Window::IsMinimized() const noexcept
{
	return IsIconic(WindowHandle.get());
}

bool Window::IsVisible() const noexcept
{
	return Visible;
}

bool Window::IsForeground() const noexcept
{
	return GetForegroundWindow() == WindowHandle.get();
}

bool Window::IsUsingRawInput() const noexcept
{
	return RawInput;
}

bool Window::IsPointInWindow(std::int32_t X, std::int32_t Y) const noexcept
{
	bool Result = false;

	RECT WindowRect = {};
	GetWindowRect(WindowHandle.get(), &WindowRect);
	HRGN Region = CreateRectRgn(0, 0, WindowRect.right - WindowRect.left, WindowRect.bottom - WindowRect.top);
	if (Region)
	{
		Result = PtInRegion(Region, X, Y);
		DeleteObject(Region);
	}

	return Result;
}

void Window::Resize(int Width, int Height)
{
	WindowWidth	 = Width;
	WindowHeight = Height;
	AspectRatio	 = static_cast<float>(WindowWidth) / static_cast<float>(WindowHeight);
}

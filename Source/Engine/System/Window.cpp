#include "Window.h"
#include "Application.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <hidusage.h>

void Window::Initialize(Application* Application, Window* Parent, HINSTANCE HInstance, const WindowDesc& Desc)
{
	this->OwningApplication = Application;
	this->HInstance			= HInstance;
	this->Desc				= Desc;

	this->Desc.Width  = this->Desc.Width == WindowDesc::SystemDefault ? CW_USEDEFAULT : this->Desc.Width;
	this->Desc.Height = this->Desc.Height == WindowDesc::SystemDefault ? CW_USEDEFAULT : this->Desc.Height;
	this->Desc.x	  = this->Desc.x == WindowDesc::SystemDefault ? CW_USEDEFAULT : this->Desc.x;
	this->Desc.y	  = this->Desc.y == WindowDesc::SystemDefault ? CW_USEDEFAULT : this->Desc.y;

	// Create window
	DWORD	ExStyle	   = 0;
	LPCWSTR ClassName  = WindowClass.data();
	LPCWSTR WindowName = Desc.Name.data();
	DWORD	Style	   = WS_OVERLAPPEDWINDOW;
	Style |= Desc.InitialSize == WindowInitialSize::Maximize ? WS_MAXIMIZE : 0;

	RECT WindowRect = { 0, 0, static_cast<LONG>(Desc.Width), static_cast<LONG>(Desc.Height) };
	AdjustWindowRect(&WindowRect, Style, FALSE);

	WindowWidth	 = WindowRect.right - WindowRect.left;
	WindowHeight = WindowRect.bottom - WindowRect.top;

	WindowHandle = ScopedWindowHandle{ CreateWindowExW(
		ExStyle,
		ClassName,
		WindowName,
		Style,
		Desc.x,
		Desc.y,
		WindowWidth,
		WindowHeight,
		Parent ? Parent->GetWindowHandle() : nullptr,
		nullptr, // No menus
		HInstance,
		Application) };
	if (!WindowHandle)
	{
		throw std::runtime_error("CreateWindowExW");
	}

	RECT ClientRect = {};
	::GetClientRect(WindowHandle.Get(), &ClientRect);
	Resize(ClientRect.right - ClientRect.left, ClientRect.bottom - ClientRect.top);

	const RAWINPUTDEVICE RawInputDevice = {
		.usUsagePage = HID_USAGE_PAGE_GENERIC,
		.usUsage	 = HID_USAGE_GENERIC_MOUSE,
		.dwFlags	 = RIDEV_INPUTSINK,
		.hwndTarget	 = WindowHandle.Get(),
	};
	if (!RegisterRawInputDevices(&RawInputDevice, 1, sizeof(RAWINPUTDEVICE)))
	{
		throw std::runtime_error("RegisterRawInputDevices");
	}
}

const WindowDesc& Window::GetDesc() const noexcept
{
	return Desc;
}

HWND Window::GetWindowHandle() const noexcept
{
	return WindowHandle.Get();
}

i32 Window::GetWidth() const noexcept
{
	return WindowWidth;
}

i32 Window::GetHeight() const noexcept
{
	return WindowHeight;
}

void Window::Show()
{
	if (!Visible)
	{
		Visible = true;
		ShowWindow(WindowHandle.Get(), SW_SHOW);
	}
}

void Window::Hide()
{
	if (Visible)
	{
		Visible = false;
		ShowWindow(WindowHandle.Get(), SW_HIDE);
	}
}

void Window::Destroy()
{
	WindowHandle.Reset();
}

void Window::SetRawInput(bool Enable)
{
	RawInput = Enable;
}

bool Window::IsMaximized() const noexcept
{
	return IsZoomed(WindowHandle.Get());
}

bool Window::IsMinimized() const noexcept
{
	return IsIconic(WindowHandle.Get());
}

bool Window::IsVisible() const noexcept
{
	return Visible;
}

bool Window::IsForeground() const noexcept
{
	return GetForegroundWindow() == WindowHandle.Get();
}

bool Window::IsUsingRawInput() const noexcept
{
	return RawInput;
}

bool Window::IsPointInWindow(i32 X, i32 Y) const noexcept
{
	bool Result		= false;
	RECT WindowRect = {};
	GetWindowRect(WindowHandle.Get(), &WindowRect);
	if (HRGN Region = CreateRectRgn(0, 0, WindowRect.right - WindowRect.left, WindowRect.bottom - WindowRect.top);
		Region)
	{
		Result = PtInRegion(Region, X, Y);
		DeleteObject(Region);
	}

	return Result;
}

void Window::Resize(i32 Width, i32 Height)
{
	WindowWidth	 = Width;
	WindowHeight = Height;
}

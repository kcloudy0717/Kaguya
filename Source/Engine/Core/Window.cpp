﻿#include "Window.h"
#include <hidusage.h>

void Window::Initialize(Application* Application, Window* Parent, HINSTANCE HInstance, const WINDOW_DESC& Desc)
{
	this->OwningApplication = Application;
	this->HInstance			= HInstance;
	this->Desc				= Desc;

	// Create window
	DWORD	ExStyle	   = 0;
	LPCWSTR WindowName = Desc.Name.data();
	DWORD	Style	   = WS_OVERLAPPEDWINDOW;
	Style |= Desc.Maximize ? WS_MAXIMIZE : 0;

	RECT WindowRect = { 0, 0, static_cast<LONG>(Desc.Width), static_cast<LONG>(Desc.Height) };
	AdjustWindowRect(&WindowRect, Style, FALSE);

	int x		 = Desc.x.value_or(CW_USEDEFAULT);
	int y		 = Desc.y.value_or(CW_USEDEFAULT);
	WindowWidth	 = WindowRect.right - WindowRect.left;
	WindowHeight = WindowRect.bottom - WindowRect.top;
	AspectRatio	 = static_cast<float>(WindowWidth) / static_cast<float>(WindowHeight);

	WindowHandle = wil::unique_hwnd(CreateWindowExW(
		ExStyle,
		Application::WindowClass,
		WindowName,
		Style,
		x,
		y,
		WindowWidth,
		WindowHeight,
		Parent ? Parent->GetWindowHandle() : nullptr,
		nullptr, // No menus
		HInstance,
		Application));
	if (!WindowHandle)
	{
		ErrorExit(TEXT("CreateWindowExW"));
	}

	RAWINPUTDEVICE RawInputDevice = {};
	RawInputDevice.usUsagePage	  = HID_USAGE_PAGE_GENERIC;
	RawInputDevice.usUsage		  = HID_USAGE_GENERIC_MOUSE;
	RawInputDevice.dwFlags		  = RIDEV_INPUTSINK;
	RawInputDevice.hwndTarget	  = WindowHandle.get();
	if (!RegisterRawInputDevices(&RawInputDevice, 1, sizeof(RAWINPUTDEVICE)))
	{
		ErrorExit(TEXT("RegisterRawInputDevices"));
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

void Window::Resize(int Width, int Height)
{
	WindowWidth	 = Width;
	WindowHeight = Height;
	AspectRatio	 = static_cast<float>(WindowWidth) / static_cast<float>(WindowHeight);
}

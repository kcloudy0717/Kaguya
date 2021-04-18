#pragma once
#include "Mouse.h"
#include "Keyboard.h"

//----------------------------------------------------------------------------------------------------
class Window;

//----------------------------------------------------------------------------------------------------
class InputHandler
{
public:
	InputHandler() = default;

	void Create(Window* pWindow);

	void Handle(const MSG* pMsg);
public:
	Mouse Mouse;
	Keyboard Keyboard;
private:
	Window* m_pWindow = nullptr;
};
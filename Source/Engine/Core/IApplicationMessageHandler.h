#pragma once
#include "InputManager.h"

class Window;

class IApplicationMessageHandler
{
public:
	virtual ~IApplicationMessageHandler() = default;

	virtual void OnKeyDown(unsigned char KeyCode, bool IsRepeat)   = 0;
	virtual void OnKeyUp(unsigned char KeyCode)					   = 0;
	virtual void OnKeyChar(unsigned char Character, bool IsRepeat) = 0;

	virtual void OnMouseMove(int X, int Y)													 = 0;
	virtual void OnMouseDown(const Window* Window, EMouseButton Button, int X, int Y)		 = 0;
	virtual void OnMouseUp(EMouseButton Button, int X, int Y)								 = 0;
	virtual void OnMouseDoubleClick(const Window* Window, EMouseButton Button, int X, int Y) = 0;
	virtual void OnMouseWheel(float Delta, int X, int Y)									 = 0;

	virtual void OnRawMouseMove(int X, int Y) = 0;

	virtual void OnWindowClose(Window* Window) = 0;

	virtual void OnWindowResize(Window* Window, int Width, int Height) = 0;
	virtual void OnWindowMove(Window* Window, int X, int Y)			   = 0;
};

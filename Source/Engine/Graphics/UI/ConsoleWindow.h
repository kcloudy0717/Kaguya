#pragma once
#include "UIWindow.h"

class ConsoleWindow : public UIWindow
{
public:
	ConsoleWindow()
		: UIWindow("Console", 0)
	{
		Clear();
	}

protected:
	void OnRender() override;

private:
	void Clear();

private:
	bool AutoScroll = true; // Keep scrolling if already at the bottom.
};

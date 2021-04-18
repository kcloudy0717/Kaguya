#pragma once
#include "UIWindow.h"

class ConsoleWindow : UIWindow
{
public:
	ConsoleWindow()
	{
		Clear();
	}

	void RenderGui();
private:
	void Clear();
private:
	bool AutoScroll = true;  // Keep scrolling if already at the bottom.
};


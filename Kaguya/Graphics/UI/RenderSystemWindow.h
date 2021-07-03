#pragma once
#include "UIWindow.h"

class RenderSystem;

class RenderSystemWindow : public UIWindow
{
public:
	void SetContext(RenderSystem* RenderSystem) { this->RenderSystem = RenderSystem; }

	void RenderGui();

private:
	RenderSystem* RenderSystem = nullptr;
};

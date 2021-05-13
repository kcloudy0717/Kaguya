#pragma once
#include "UIWindow.h"

class RenderSystem;

class RenderSystemWindow : public UIWindow
{
public:
	void SetContext(RenderSystem* pRenderSystem)
	{
		this->m_pRenderSystem = pRenderSystem;
	}

	void RenderGui();
private:
	RenderSystem* m_pRenderSystem = nullptr;
};

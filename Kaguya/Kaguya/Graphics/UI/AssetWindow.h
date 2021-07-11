#pragma once
#include "UIWindow.h"

#include "../Scene/Scene.h"
#include "../Scene/Entity.h"

class AssetWindow : public UIWindow
{
public:
	void SetContext(Scene* pScene) { m_pScene = pScene; }

	void RenderGui();

private:
	Scene* m_pScene = nullptr;
};

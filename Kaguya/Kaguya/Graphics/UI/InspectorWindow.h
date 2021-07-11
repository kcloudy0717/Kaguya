#pragma once
#include "UIWindow.h"

#include "../Scene/Scene.h"
#include "../Scene/Entity.h"

class InspectorWindow : public UIWindow
{
public:
	void SetContext(Scene* pScene, Entity Entity)
	{
		m_pScene		 = pScene;
		m_SelectedEntity = Entity;
	}

	void RenderGui();

private:
	Scene* m_pScene			= nullptr;
	Entity m_SelectedEntity = {};
};

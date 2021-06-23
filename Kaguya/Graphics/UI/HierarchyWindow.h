#pragma once
#include "UIWindow.h"
#include "../Scene/Scene.h"
#include "../Scene/Entity.h"

class HierarchyWindow : public UIWindow
{
public:
	void SetContext(Scene* pScene)
	{
		m_pScene		 = pScene;
		m_SelectedEntity = {};
	}

	void RenderGui();

	Entity GetSelectedEntity() const { return m_SelectedEntity; }

	void SetSelectedEntity(Entity Entity) { m_SelectedEntity = Entity; }

private:
	Scene* m_pScene			= nullptr;
	Entity m_SelectedEntity = {};
};

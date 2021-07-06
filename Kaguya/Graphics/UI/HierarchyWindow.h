#pragma once
#include "UIWindow.h"
#include "../Scene/Scene.h"
#include "../Scene/Entity.h"

class HierarchyWindow : public UIWindow
{
public:
	void SetContext(Scene* pScene)
	{
		this->pScene   = pScene;
		SelectedEntity = {};
	}

	void RenderGui();

	Entity GetSelectedEntity() const { return SelectedEntity; }

	void SetSelectedEntity(Entity Entity) { SelectedEntity = Entity; }

private:
	Scene* pScene		  = nullptr;
	Entity SelectedEntity = {};
};

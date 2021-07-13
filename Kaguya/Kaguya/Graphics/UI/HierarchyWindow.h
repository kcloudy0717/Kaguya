#pragma once
#include "UIWindow.h"
#include <World/World.h>
#include <World/Entity.h>

class HierarchyWindow : public UIWindow
{
public:
	void SetContext(World* pWorld)
	{
		this->pWorld   = pWorld;
		SelectedEntity = {};
	}

	void RenderGui();

	Entity GetSelectedEntity() const { return SelectedEntity; }

	void SetSelectedEntity(Entity Entity) { SelectedEntity = Entity; }

private:
	World* pWorld		  = nullptr;
	Entity SelectedEntity = {};
};

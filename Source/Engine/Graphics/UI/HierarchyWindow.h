#pragma once
#include "UIWindow.h"
#include <World/World.h>
#include <World/Entity.h>

class HierarchyWindow : public UIWindow
{
public:
	HierarchyWindow()
		: UIWindow("Hierarchy", 0)
	{
	}

	void SetContext(World* pWorld)
	{
		this->pWorld   = pWorld;
		SelectedEntity = {};
	}

	Entity GetSelectedEntity() const { return SelectedEntity; }

	void SetSelectedEntity(Entity Entity) { SelectedEntity = Entity; }

protected:
	void OnRender() override;

private:
	World* pWorld		  = nullptr;
	Entity SelectedEntity = {};
};

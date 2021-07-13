#pragma once
#include "UIWindow.h"

#include <World/World.h>
#include <World/Entity.h>

class InspectorWindow : public UIWindow
{
public:
	void SetContext(World* pWorld, Entity Entity)
	{
		this->pWorld   = pWorld;
		SelectedEntity = Entity;
	}

	void RenderGui();

private:
	World* pWorld		  = nullptr;
	Entity SelectedEntity = {};
};

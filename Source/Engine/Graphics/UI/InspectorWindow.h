#pragma once
#include "UIWindow.h"
#include <World/World.h>
#include <World/Entity.h>

class InspectorWindow : public UIWindow
{
public:
	InspectorWindow()
		: UIWindow("Inspector", 0)
	{
	}

	void SetContext(World* pWorld, Entity Entity)
	{
		this->pWorld = pWorld;
		if (Entity)
		{
			SelectedEntity = Entity;
		}
	}

protected:
	void OnRender() override;

private:
	World* pWorld		  = nullptr;
	Entity SelectedEntity = {};
};

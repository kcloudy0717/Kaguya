#pragma once
#include "UIWindow.h"
#include <World/World.h>
#include <World/Actor.h>

class InspectorWindow : public UIWindow
{
public:
	InspectorWindow()
		: UIWindow("Inspector", 0)
	{
	}

	void SetContext(World* pWorld, Actor Actor)
	{
		this->pWorld = pWorld;
		if (Actor)
		{
			SelectedActor = Actor;
		}
	}

protected:
	void OnRender() override;

private:
	World* pWorld		  = nullptr;
	Actor  SelectedActor = {};
};

#pragma once
#include "UIWindow.h"

#include <World/World.h>
#include <World/Entity.h>

class AssetWindow : public UIWindow
{
public:
	void SetContext(World* pWorld) { this->pWorld = pWorld; }

	void RenderGui();

private:
	World* pWorld = nullptr;
};

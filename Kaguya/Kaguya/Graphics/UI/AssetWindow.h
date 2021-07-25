#pragma once
#include "UIWindow.h"

#include <World/World.h>
#include <World/Entity.h>

class AssetWindow : public UIWindow
{
public:
	AssetWindow()
		: UIWindow("Asset", 0)
	{

	}

	void SetContext(World* pWorld) { this->pWorld = pWorld; }

protected:
	void OnRender() override;

private:
	World* pWorld = nullptr;
};

#pragma once
#include "UIWindow.h"

#include <World/World.h>
#include <World/Actor.h>

class AssetWindow : public UIWindow
{
public:
	AssetWindow()
		: UIWindow("Asset", 0)
	{
		ValidImageHandles.reserve(1024);
		ValidMeshHandles.reserve(1024);
	}

	void SetContext(World* pWorld) { this->pWorld = pWorld; }

protected:
	void OnRender() override;

private:
	World*					 pWorld = nullptr;
	std::vector<AssetHandle> ValidImageHandles;
	std::vector<AssetHandle> ValidMeshHandles;
};

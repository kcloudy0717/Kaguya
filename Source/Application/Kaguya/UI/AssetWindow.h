#pragma once
#include "UIWindow.h"

#include <Core/World/World.h>
#include <Core/World/Actor.h>

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

	MeshImportOptions	 MeshOptions	= {};
	TextureImportOptions TextureOptions = {};
};

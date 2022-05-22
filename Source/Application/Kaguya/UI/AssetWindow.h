#pragma once
#include "UIWindow.h"
#include <Core/World/World.h>

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
	World*							pWorld = nullptr;
	std::vector<Asset::AssetHandle> ValidImageHandles;
	std::vector<Asset::AssetHandle> ValidMeshHandles;

	Asset::MeshImportOptions	MeshOptions	   = {};
	Asset::TextureImportOptions TextureOptions = {};
};

#pragma once
#include "Component.h"
#include "../../Asset/Mesh.h"
#include "../../Asset/AssetCache.h"

struct MeshFilter : Component
{
	// Key into the MeshCache
	UINT64 Key = 0;
	AssetHandle<Asset::Mesh> Mesh = {};
};
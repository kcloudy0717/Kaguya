#pragma once

struct MeshFilter : Component
{
	// Key into the MeshCache
	UINT64					 Key  = 0;
	AssetHandle<Asset::Mesh> Mesh = {};
};

#pragma once

struct MeshFilter : Component
{
	// Key into the MeshCache
	std::string				 Path;
	UINT64					 Key  = 0;
	AssetHandle<Asset::Mesh> Mesh = {};
};

REGISTER_CLASS_ATTRIBUTES(MeshFilter, CLASS_ATTRIBUTE(MeshFilter, Path))

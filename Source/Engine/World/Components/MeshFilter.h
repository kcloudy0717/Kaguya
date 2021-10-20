#pragma once

struct MeshFilter : Component
{
	MeshFilter() noexcept = default;
	MeshFilter(AssetHandle Handle)
		: Handle(Handle)
		, HandleId(Handle.Id)
	{
	}

	AssetHandle Handle	 = {};
	uint32_t	HandleId = UINT32_MAX;
	Mesh*		Mesh	 = nullptr;
};

REGISTER_CLASS_ATTRIBUTES(MeshFilter, "MeshFilter", CLASS_ATTRIBUTE(MeshFilter, HandleId))

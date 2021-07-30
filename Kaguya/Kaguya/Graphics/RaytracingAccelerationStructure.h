#pragma once
#include "World/World.h"
#include "World/Entity.h"

#define RAYTRACING_INSTANCEMASK_ALL	   (0xff)
#define RAYTRACING_INSTANCEMASK_OPAQUE (1 << 0)
#define RAYTRACING_INSTANCEMASK_LIGHT  (1 << 1)

class RaytracingAccelerationStructure
{
public:
	RaytracingAccelerationStructure() noexcept = default;
	RaytracingAccelerationStructure(UINT NumHitGroups);

	void Initialize();

	operator auto() const { return TLASResult.GetGPUVirtualAddress(); }

	auto size() const { return TopLevelAccelerationStructure.size(); }

	bool empty() const { return TopLevelAccelerationStructure.empty(); }

	void clear()
	{
		TopLevelAccelerationStructure.clear();
		MeshRenderers.clear();
		ReferencedGeometries.clear();
		InstanceContributionToHitGroupIndex = 0;
	}

	void AddInstance(const Transform& Transform, MeshRenderer* pMeshRenderer);

	void Build(CommandContext& Context);

	UINT NumHitGroups = 0;

	TopLevelAccelerationStructure	   TopLevelAccelerationStructure;
	std::vector<MeshRenderer*>		   MeshRenderers;
	std::set<AssetHandle<Asset::Mesh>> ReferencedGeometries;
	UINT							   InstanceContributionToHitGroupIndex = 0;

	Buffer							TLASScratch;
	ASBuffer						TLASResult;
	Buffer							InstanceDescs;
	D3D12_RAYTRACING_INSTANCE_DESC* pInstanceDescs = nullptr;
};

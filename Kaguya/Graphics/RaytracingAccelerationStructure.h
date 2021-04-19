#pragma once
#include "Scene/Scene.h"
#include "Scene/Entity.h"

#define RAYTRACING_INSTANCEMASK_ALL 	(0xff)
#define RAYTRACING_INSTANCEMASK_OPAQUE 	(1 << 0)
#define RAYTRACING_INSTANCEMASK_LIGHT	(1 << 1)

class RaytracingAccelerationStructure
{
public:
	void Create(UINT NumHitGroups);

	operator auto() const
	{
		return m_TLASResult->pResource->GetGPUVirtualAddress();
	}

	auto size() const
	{
		return m_TopLevelAccelerationStructure.size();
	}

	bool empty() const
	{
		return m_TopLevelAccelerationStructure.empty();
	}

	void clear()
	{
		m_TopLevelAccelerationStructure.clear();
		m_MeshRenderers.clear();
		m_InstanceContributionToHitGroupIndex = 0;
	}

	void AddInstance(MeshRenderer* pMeshRenderer);

	void Build(CommandList& CommandList);

private:
	UINT m_NumHitGroups = 0;

	TopLevelAccelerationStructure m_TopLevelAccelerationStructure;
	std::vector<MeshRenderer*> m_MeshRenderers;
	UINT m_InstanceContributionToHitGroupIndex = 0;

	std::shared_ptr<Resource> m_TLASScratch, m_TLASResult;
	std::shared_ptr<Resource> m_InstanceDescs;
	D3D12_RAYTRACING_INSTANCE_DESC* m_pInstanceDescs = nullptr;

	friend class PathIntegrator;
	friend class Picking;
};
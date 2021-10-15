#pragma once
#include "World/World.h"

#define RAYTRACING_INSTANCEMASK_ALL	   (0xff)
// These are not used atm, I haven't found a use case for them (yet)
#define RAYTRACING_INSTANCEMASK_OPAQUE (1 << 0)
#define RAYTRACING_INSTANCEMASK_LIGHT  (1 << 1)

class RaytracingAccelerationStructure
{
public:
	RaytracingAccelerationStructure() noexcept = default;
	RaytracingAccelerationStructure(UINT NumHitGroups, size_t NumInstances);

	void Initialize();

	operator auto() const { return TlasResult.GetGpuVirtualAddress(); }

	[[nodiscard]] bool IsValid() const noexcept { return !TopLevelAccelerationStructure.empty(); }

	void Reset();

	void AddInstance(const Transform& Transform, MeshRenderer* MeshRenderer);

	void Build(D3D12CommandContext& Context);

	// Call this after the command context for build has been executed, this will
	// update internal BLAS address
	void PostBuild(D3D12SyncHandle SyncHandle);

	UINT   NumHitGroups = 0;
	size_t NumInstances = 0;

	D3D12RaytracingAccelerationStructureManager Manager;

	D3D12RaytracingScene	   TopLevelAccelerationStructure;
	std::vector<MeshRenderer*> MeshRenderers;
	std::set<Mesh*>			   ReferencedGeometries;
	UINT					   CurrentInstanceID						  = 0;
	UINT					   CurrentInstanceContributionToHitGroupIndex = 0;

	D3D12Buffer	  TlasScratch;
	D3D12ASBuffer TlasResult;
	D3D12Buffer	  InstanceDescs;
};

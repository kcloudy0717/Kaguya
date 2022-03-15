#pragma once
#include "RHI/RHI.h"
#include "Core/CoreDefines.h"
#include "Core/World/World.h"

#define RAYTRACING_INSTANCEMASK_ALL (0xff)

class RaytracingAccelerationStructure
{
public:
	RaytracingAccelerationStructure() noexcept = default;
	RaytracingAccelerationStructure(RHI::D3D12Device* Device, UINT NumHitGroups, size_t NumInstances);

	const RHI::D3D12ShaderResourceView& GetShaderResourceView() const { return IsValid() ? SRV : Null; }

	[[nodiscard]] bool IsValid() const noexcept { return !TopLevelAccelerationStructure.empty(); }

	void Reset();

	void AddInstance(const Transform& Transform, StaticMeshComponent* StaticMesh);

	void Build(RHI::D3D12CommandContext& Context);

	// Call this after the command context for Build has been executed, this will
	// update internal BLAS address
	void PostBuild(RHI::D3D12SyncHandle SyncHandle);

	RHI::D3D12Device* Device	   = nullptr;
	UINT			  NumHitGroups = 0;
	size_t			  NumInstances = 0;

	RHI::D3D12RaytracingManager Manager;

	RHI::D3D12RaytracingScene		  TopLevelAccelerationStructure;
	std::vector<StaticMeshComponent*> StaticMeshes;
	std::set<Asset::Mesh*>			  ReferencedGeometries;
	UINT							  CurrentInstanceID							 = 0;
	UINT							  CurrentInstanceContributionToHitGroupIndex = 0;

	RHI::D3D12Buffer   TlasScratch;
	RHI::D3D12ASBuffer TlasResult;
	RHI::D3D12Buffer   InstanceDescs;

	RHI::D3D12ShaderResourceView Null;
	RHI::D3D12ShaderResourceView SRV;
};

#pragma once
#include "RenderDevice.h"
#include "RaytracingAccelerationStructure.h"

class Picking
{
public:
	void Create();

	void UpdateShaderTable(const RaytracingAccelerationStructure& Scene, CommandContext& Context);

	void ShootPickingRay(
		D3D12_GPU_VIRTUAL_ADDRESS			   SystemConstants,
		const RaytracingAccelerationStructure& Scene,
		CommandContext&						   Context);

	std::optional<Entity> GetSelectedEntity();

private:
	RootSignature			GlobalRS;
	RaytracingPipelineState RTPSO;

	Buffer Result;
	Buffer Readback;

	RaytracingShaderBindingTable ShaderBindingTable;
	RaytracingShaderTable<void>* RayGenerationShaderTable;
	RaytracingShaderTable<void>* MissShaderTable;
	RaytracingShaderTable<void>* HitGroupShaderTable;

	std::vector<Entity> Entities;
};

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
	RootSignature			m_GlobalRS;
	RaytracingPipelineState m_RTPSO;

	Buffer m_RayGenerationSBT;
	Buffer m_MissSBT;
	Buffer m_HitGroupSBT;

	Buffer m_Result;
	Buffer m_Readback;

	RaytracingShaderTable<void> m_RayGenerationShaderTable;
	RaytracingShaderTable<void> m_MissShaderTable;
	RaytracingShaderTable<void> m_HitGroupShaderTable;
	std::vector<Entity>			m_Entities;
};

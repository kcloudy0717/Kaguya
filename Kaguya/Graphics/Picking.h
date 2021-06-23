#pragma once
#include "RenderDevice.h"
#include "RaytracingAccelerationStructure.h"

class Picking
{
public:
	void Create();

	void UpdateShaderTable(const RaytracingAccelerationStructure& Scene, CommandList& CommandList);
	void ShootPickingRay(
		D3D12_GPU_VIRTUAL_ADDRESS			   SystemConstants,
		const RaytracingAccelerationStructure& Scene,
		CommandList&						   CommandList);

	std::optional<Entity> GetSelectedEntity();

private:
	RootSignature			m_GlobalRS;
	RaytracingPipelineState m_RTPSO;

	std::shared_ptr<Resource> m_RayGenerationSBT;
	std::shared_ptr<Resource> m_MissSBT;
	std::shared_ptr<Resource> m_HitGroupSBT;

	std::shared_ptr<Resource> m_Result;
	std::shared_ptr<Resource> m_Readback;

	RaytracingShaderTable<void> m_RayGenerationShaderTable;
	RaytracingShaderTable<void> m_MissShaderTable;
	RaytracingShaderTable<void> m_HitGroupShaderTable;
	std::vector<Entity>			m_Entities;
};

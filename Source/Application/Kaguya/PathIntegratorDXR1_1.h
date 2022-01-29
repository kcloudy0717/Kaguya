#pragma once
#include "PathIntegratorDXR1_0.h"

class PathIntegratorDXR1_1 final : public Renderer
{
public:
	using Renderer::Renderer;

private:
	void Initialize() override;
	void Destroy() override;
	void Render(World* World, RHI::D3D12CommandContext& Context) override;

private:
	RaytracingAccelerationStructure AccelerationStructure;

	// Temporal accumulation
	UINT NumTemporalSamples = 0;
	UINT FrameCounter		= 0;

	PathIntegratorState PathIntegratorState;

	RHI::D3D12Buffer Materials;
	Hlsl::Material*	 pMaterial = nullptr;
	RHI::D3D12Buffer Lights;
	Hlsl::Light*	 pLights = nullptr;
	RHI::D3D12Buffer Meshes;
	Hlsl::Mesh*		 pMeshes = nullptr;

	UINT NumMaterials = 0, NumLights = 0, NumMeshes = 0;

	std::vector<Hlsl::Mesh> HlslMeshes;
};

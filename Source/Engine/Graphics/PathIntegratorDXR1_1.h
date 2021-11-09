#pragma once
#include "PathIntegrator.h"

class PathIntegratorDXR1_1 final : public Renderer
{
public:
	using Renderer::Renderer;

private:
	void SetViewportResolution(uint32_t Width, uint32_t Height) override;
	void Initialize() override;
	void Destroy() override;
	void Render(World* World, D3D12CommandContext& Context) override;

private:
	RaytracingAccelerationStructure AccelerationStructure;

	// Temporal accumulation
	UINT NumTemporalSamples = 0;
	UINT FrameCounter		= 0;

	PathIntegratorState PathIntegratorState;

	D3D12Buffer		Materials;
	Hlsl::Material* pMaterial = nullptr;
	D3D12Buffer		Lights;
	Hlsl::Light*	pLights		 = nullptr;
	UINT			NumMaterials = 0, NumLights = 0;
};

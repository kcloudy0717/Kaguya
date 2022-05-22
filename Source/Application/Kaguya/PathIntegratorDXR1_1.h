#pragma once
#include "PathIntegratorDXR1_0.h"
#include "Bloom.h"
#include "Tonemap.h"

class PathIntegratorDXR1_1 final : public Renderer
{
public:
	PathIntegratorDXR1_1(
		RHI::D3D12Device* Device,
		ShaderCompiler*	  Compiler);

private:
	void RenderOptions() override;
	void Render(World* World, WorldRenderView* WorldRenderView, RHI::D3D12CommandContext& Context) override;

	void ResetAccumulation();

private:
	RaytracingAccelerationStructure AccelerationStructure;

	// Temporal accumulation
	UINT NumTemporalSamples = 0;
	UINT FrameCounter		= 0;

	PathIntegratorSettings Settings;

	BloomSettings	Bloom;
	TonemapSettings Tonemap;
};

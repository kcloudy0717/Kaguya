#pragma once
#include "Renderer.h"
#include "RaytracingAccelerationStructure.h"

struct PathIntegratorSettings
{
	static constexpr UINT MinimumDepth = 1;
	static constexpr UINT MaximumDepth = 32;

	float SkyIntensity = 1.0f;
	UINT  MaxDepth	   = 16;
	bool  Antialiasing = true;
};

class PathIntegratorDXR1_0 final : public Renderer
{
public:
	PathIntegratorDXR1_0(
		RHI::D3D12Device* Device,
		ShaderCompiler*	  Compiler);

private:
	void RenderOptions() override;
	void Render(World* World, WorldRenderView* WorldRenderView, RHI::D3D12CommandContext& Context) override;

private:
	RaytracingAccelerationStructure AccelerationStructure;

	// Temporal accumulation
	UINT NumTemporalSamples = 0;
	UINT FrameCounter		= 0;

	PathIntegratorSettings Settings;

	// Pad local root arguments explicitly
	struct RootArgument
	{
		UINT64					  MaterialIndex : 32;
		UINT64					  Padding		: 32;
		D3D12_GPU_VIRTUAL_ADDRESS VertexBuffer;
		D3D12_GPU_VIRTUAL_ADDRESS IndexBuffer;
	};

	RHI::D3D12RaytracingShaderBindingTable		   ShaderBindingTable;
	RHI::D3D12RaytracingShaderTable<void>*		   RayGenerationShaderTable = nullptr;
	RHI::D3D12RaytracingShaderTable<void>*		   MissShaderTable			= nullptr;
	RHI::D3D12RaytracingShaderTable<RootArgument>* HitGroupShaderTable		= nullptr;
};

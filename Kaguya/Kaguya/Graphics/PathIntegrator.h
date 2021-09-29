#pragma once
#include "Renderer.h"
#include "RaytracingAccelerationStructure.h"

struct PathIntegratorState
{
	static constexpr UINT MinimumDepth = 1;
	static constexpr UINT MaximumDepth = 32;

	float SkyIntensity = 1.0f;
	UINT  MaxDepth	   = 16;
};

enum class EFSRQualityMode
{
	Ultra,
	Standard,
	Balanced,
	Performance
};

struct FSRState
{
	bool Enable = true;

	EFSRQualityMode QualityMode = EFSRQualityMode::Ultra;

	UINT ViewportWidth;
	UINT ViewportHeight;

	UINT  RenderWidth;
	UINT  RenderHeight;
	float RCASAttenuation = 0.0f;
};

class PathIntegrator : public Renderer
{
public:
	void* GetViewportDescriptor() override;

private:
	void SetViewportResolution(uint32_t Width, uint32_t Height) override;
	void Initialize() override;
	void Render(World* World, D3D12CommandContext& Context) override;

private:
	D3D12RenderPass TonemapRenderPass;

	RaytracingAccelerationStructure AccelerationStructure;

	// Temporal accumulation
	UINT NumTemporalSamples = 0;
	UINT FrameCounter		= 0;

	PathIntegratorState PathIntegratorState;
	FSRState			FSRState;

	D3D12Buffer		Materials;
	Hlsl::Material* pMaterial = nullptr;
	D3D12Buffer		Lights;
	Hlsl::Light*	pLights		 = nullptr;
	UINT			NumMaterials = 0, NumLights = 0;

	// Pad local root arguments explicitly
	struct RootArgument
	{
		UINT64					  MaterialIndex : 32;
		UINT64					  Padding		: 32;
		D3D12_GPU_VIRTUAL_ADDRESS VertexBuffer;
		D3D12_GPU_VIRTUAL_ADDRESS IndexBuffer;
	};

	D3D12RaytracingShaderBindingTable		  ShaderBindingTable;
	D3D12RaytracingShaderTable<void>*		  RayGenerationShaderTable;
	D3D12RaytracingShaderTable<void>*		  MissShaderTable;
	D3D12RaytracingShaderTable<RootArgument>* HitGroupShaderTable;

	RenderResourceHandle Viewport;
};

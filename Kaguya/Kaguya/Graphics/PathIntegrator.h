#pragma once
#include "Renderer.h"

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

	int ViewportWidth;
	int ViewportHeight;

	int	  RenderWidth;
	int	  RenderHeight;
	float RCASAttenuation = 0.0f;
};

class PathIntegrator : public Renderer
{
public:
	PathIntegrator(World* pWorld)
		: Renderer(pWorld)
	{
	}

	void* GetViewportDescriptor() override;

private:
	void SetViewportResolution(uint32_t Width, uint32_t Height) override;
	void Initialize() override;
	void Render(CommandContext& Context) override;

private:
	RaytracingAccelerationStructure AccelerationStructure;

	RaytracingAccelerationStructureManager Manager;

	struct Settings
	{
		inline static UINT NumAccumulatedSamples = 0;
	};
	PathIntegratorState PathIntegratorState;
	FSRState			FSRState;

	Buffer			Materials;
	HLSL::Material* pMaterials = nullptr;
	Buffer			Lights;
	HLSL::Light*	pLights		 = nullptr;
	UINT			NumMaterials = 0, NumLights = 0;

	// Pad local root arguments explicitly
	struct RootArgument
	{
		UINT64					  MaterialIndex : 32;
		UINT64					  Padding		: 32;
		D3D12_GPU_VIRTUAL_ADDRESS VertexBuffer;
		D3D12_GPU_VIRTUAL_ADDRESS IndexBuffer;
	};

	RaytracingShaderBindingTable		 ShaderBindingTable;
	RaytracingShaderTable<void>*		 RayGenerationShaderTable;
	RaytracingShaderTable<void>*		 MissShaderTable;
	RaytracingShaderTable<RootArgument>* HitGroupShaderTable;
};

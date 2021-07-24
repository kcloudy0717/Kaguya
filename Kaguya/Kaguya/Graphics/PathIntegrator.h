#pragma once
#include "RenderDevice.h"
#include "RaytracingAccelerationStructure.h"

struct PathIntegratorState
{
	static constexpr UINT MinimumDepth = 1;
	static constexpr UINT MaximumDepth = 32;

	float SkyIntensity = 1.0f;
	UINT  MaxDepth	   = 16;
};

class PathIntegrator_DXR_1_0
{
public:
	struct Settings
	{
		inline static UINT NumAccumulatedSamples = 0;
	};

	static constexpr UINT NumHitGroups = 1;

	PathIntegrator_DXR_1_0() noexcept = default;

	void Initialize(RenderDevice& RenderDevice);

	void SetResolution(UINT Width, UINT Height);

	void Reset();

	void UpdateShaderTable(
		const RaytracingAccelerationStructure& RaytracingAccelerationStructure,
		CommandContext&						   Context);

	void Render(
		const PathIntegratorState&			   State,
		D3D12_GPU_VIRTUAL_ADDRESS			   SystemConstants,
		const RaytracingAccelerationStructure& RaytracingAccelerationStructure,
		D3D12_GPU_VIRTUAL_ADDRESS			   Materials,
		D3D12_GPU_VIRTUAL_ADDRESS			   Lights,
		CommandContext&						   Context);

	const ShaderResourceView& GetSRV() const { return SRV; }
	ID3D12Resource*			  GetRenderTarget() const { return RenderTarget.GetResource(); }

private:
	UINT Width = 0, Height = 0;

	RootSignature			GlobalRS, LocalHitGroupRS;
	RaytracingPipelineState RTPSO;

	Texture RenderTarget;

	UnorderedAccessView UAV;
	ShaderResourceView	SRV;

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

class PathIntegrator_DXR_1_1
{
};

#pragma once
#include "RenderDevice.h"
#include "RaytracingAccelerationStructure.h"

class PathIntegrator
{
public:
	struct Settings
	{
		static constexpr UINT MinimumDepth = 1;
		static constexpr UINT MaximumDepth = 32;

		inline static UINT MaxDepth;
		inline static UINT NumAccumulatedSamples;

		static void RestoreDefaults()
		{
			MaxDepth			  = 16;
			NumAccumulatedSamples = 0;
		}

		static void RenderGui();
	};

	static constexpr UINT NumHitGroups = 1;

	PathIntegrator() noexcept = default;

	PathIntegrator(RenderDevice& RenderDevice);

	void SetResolution(UINT Width, UINT Height);

	void Reset();

	void UpdateShaderTable(
		const RaytracingAccelerationStructure& RaytracingAccelerationStructure,
		CommandContext&						   Context);

	void Render(
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
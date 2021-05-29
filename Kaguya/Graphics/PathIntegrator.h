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
			MaxDepth = 16;
			NumAccumulatedSamples = 0;
		}

		static void RenderGui();
	};

	static constexpr UINT NumHitGroups = 1;

	PathIntegrator() noexcept = default;
	PathIntegrator(
		_In_ RenderDevice& RenderDevice);

	void SetResolution(
		_In_ UINT Width,
		_In_ UINT Height);

	void Reset();

	void UpdateShaderTable(
		_In_ const RaytracingAccelerationStructure& RaytracingAccelerationStructure,
		_In_ CommandList& CommandList);

	void Render(
		_In_ D3D12_GPU_VIRTUAL_ADDRESS SystemConstants,
		_In_ const RaytracingAccelerationStructure& RaytracingAccelerationStructure,
		_In_ D3D12_GPU_VIRTUAL_ADDRESS Materials,
		_In_ D3D12_GPU_VIRTUAL_ADDRESS Lights,
		_In_ CommandList& CommandList);

	Descriptor GetSRV() const { return m_SRV; }
	ID3D12Resource* GetRenderTarget() const { return m_RenderTarget->pResource.Get(); }

private:
	UINT m_Width = 0, m_Height = 0;

	RootSignature m_GlobalRS, m_LocalHitGroupRS;
	RaytracingPipelineState m_RTPSO;
	Descriptor m_UAV;
	Descriptor m_SRV;

	std::shared_ptr<Resource> m_RenderTarget;
	std::shared_ptr<Resource> m_RayGenerationSBT;
	std::shared_ptr<Resource> m_MissSBT;
	std::shared_ptr<Resource> m_HitGroupSBT;

	// Pad local root arguments explicitly
	struct RootArgument
	{
		UINT64 MaterialIndex : 32;
		UINT64 Padding : 32;
		D3D12_GPU_VIRTUAL_ADDRESS VertexBuffer;
		D3D12_GPU_VIRTUAL_ADDRESS IndexBuffer;
	};

	RaytracingShaderTable<void> m_RayGenerationShaderTable;
	RaytracingShaderTable<void> m_MissShaderTable;
	RaytracingShaderTable<RootArgument> m_HitGroupShaderTable;
};
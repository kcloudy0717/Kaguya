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

		inline static PathIntegrator* pPathIntegrator = nullptr;

		static void RestoreDefaults()
		{
			MaxDepth = 16;
			NumAccumulatedSamples = 0;
		}

		static void RenderGui();
	};

	static constexpr UINT NumHitGroups = 1;

	void Create();

	void SetResolution(UINT Width, UINT Height);

	void Reset();

	void UpdateShaderTable(const RaytracingAccelerationStructure& RaytracingAccelerationStructure,
		CommandList& CommandList);

	void Render(D3D12_GPU_VIRTUAL_ADDRESS SystemConstants,
		const RaytracingAccelerationStructure& RaytracingAccelerationStructure,
		D3D12_GPU_VIRTUAL_ADDRESS Materials,
		D3D12_GPU_VIRTUAL_ADDRESS Lights,
		CommandList& CommandList);

	auto GetSRV() const
	{
		return m_SRV;
	}

	auto GetRenderTarget() const
	{
		return m_RenderTarget->pResource.Get();
	}
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

	ShaderTable<void> m_RayGenerationShaderTable;
	ShaderTable<void> m_MissShaderTable;
	ShaderTable<RootArgument> m_HitGroupShaderTable;
};
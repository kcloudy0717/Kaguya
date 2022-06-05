#pragma once
#include "Renderer.h"

#define USE_MESH_SHADERS 0

class DeferredRenderer final : public Renderer
{
public:
	DeferredRenderer(
		RHI::D3D12Device* Device,
		ShaderCompiler*	  Compiler);

private:
	void RenderOptions() override;
	void Render(World* World, WorldRenderView* WorldRenderView, RHI::D3D12CommandContext& Context) override;

private:
#pragma pack(push, 4)
	struct CommandSignatureParams
	{
		u32							 MeshIndex;
		D3D12_VERTEX_BUFFER_VIEW	 VertexBuffer;
		D3D12_INDEX_BUFFER_VIEW		 IndexBuffer;
		D3D12_DRAW_INDEXED_ARGUMENTS DrawIndexedArguments;
	};
#pragma pack(pop)

	static constexpr u64 TotalCommandBufferSizeInBytes = World::MeshLimit * sizeof(CommandSignatureParams);
	static constexpr u64 CommandBufferCounterOffset	   = AlignUp(TotalCommandBufferSizeInBytes, D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT);

	RHI::D3D12CommandSignature CommandSignature;

	Shader					GBufferVS;
	Shader					GBufferPS;
	Shader					ShadingCS;

	RHI::D3D12RootSignature GBufferRS;
	RHI::D3D12RootSignature ShadingRS;

	RHI::D3D12PipelineState GBufferPSO;
	RHI::D3D12PipelineState ShadingPSO;

	RHI::D3D12Buffer			  IndirectCommandBuffer;
	RHI::D3D12UnorderedAccessView IndirectCommandBufferUav;

	int ViewMode = 0;
};

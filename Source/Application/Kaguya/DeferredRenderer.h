#pragma once
#include "Renderer.h"
#include "Core/CoreDefines.h"

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
#if USE_MESH_SHADERS
	struct CommandSignatureParams
	{
		u32							  MeshIndex;
		D3D12_GPU_VIRTUAL_ADDRESS	  Vertices;
		D3D12_GPU_VIRTUAL_ADDRESS	  Meshlets;
		D3D12_GPU_VIRTUAL_ADDRESS	  UniqueVertexIndices;
		D3D12_GPU_VIRTUAL_ADDRESS	  PrimitiveIndices;
		D3D12_DISPATCH_MESH_ARGUMENTS DispatchMeshArguments;
	};
#else
	struct CommandSignatureParams
	{
		u32							 MeshIndex;
		D3D12_VERTEX_BUFFER_VIEW	 VertexBuffer;
		D3D12_INDEX_BUFFER_VIEW		 IndexBuffer;
		D3D12_DRAW_INDEXED_ARGUMENTS DrawIndexedArguments;
	};
#endif
#pragma pack(pop)

	static constexpr u64 TotalCommandBufferSizeInBytes = World::MeshLimit * sizeof(CommandSignatureParams);
	static constexpr u64 CommandBufferCounterOffset	   = AlignUp(TotalCommandBufferSizeInBytes, D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT);

	RHI::D3D12CommandSignature CommandSignature;

	RHI::D3D12Buffer			  IndirectCommandBuffer;
	RHI::D3D12UnorderedAccessView IndirectCommandBufferUav;

	int ViewMode = 0;
};

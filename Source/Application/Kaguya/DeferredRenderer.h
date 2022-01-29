#pragma once
#include "Renderer.h"
#include "Core/CoreDefines.h"

#define USE_MESH_SHADERS 0

class DeferredRenderer final : public Renderer
{
public:
	using Renderer::Renderer;

private:
	void Initialize() override;
	void Destroy() override;
	void Render(World* World, RHI::D3D12CommandContext& Context) override;

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
	RHI::D3D12UnorderedAccessView UAV;

	std::vector<StaticMeshComponent*> StaticMeshes;
	std::vector<Hlsl::Mesh>			  HlslMeshes;

	RHI::D3D12Buffer Materials;
	Hlsl::Material*	 pMaterial = nullptr;
	RHI::D3D12Buffer Lights;
	Hlsl::Light*	 pLights = nullptr;
	RHI::D3D12Buffer Meshes;
	Hlsl::Mesh*		 pMeshes = nullptr;

	u32 NumMaterials = 0, NumLights = 0, NumMeshes = 0;

	int ViewMode = 0;
};

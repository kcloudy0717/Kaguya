#pragma once
#include "Renderer.h"

class DeferredRenderer final : public Renderer
{
public:
	void* GetViewportDescriptor() override;

private:
	void SetViewportResolution(uint32_t Width, uint32_t Height) override;
	void Initialize() override;
	void Render(World* World, D3D12CommandContext& Context) override;

private:
#pragma pack(push, 4)
	struct CommandSignatureParams
	{
		UINT						 MeshIndex;
		D3D12_VERTEX_BUFFER_VIEW	 VertexBuffer;
		D3D12_INDEX_BUFFER_VIEW		 IndexBuffer;
		D3D12_DRAW_INDEXED_ARGUMENTS DrawIndexedArguments;
	};
#pragma pack(pop)

	static constexpr UINT64 TotalCommandBufferSizeInBytes = World::InstanceLimit * sizeof(CommandSignatureParams);
	static constexpr UINT64 CommandBufferCounterOffset =
		AlignUp(TotalCommandBufferSizeInBytes, D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT);

	D3D12CommandSignature CommandSignature;

	D3D12Buffer				 IndirectCommandBuffer;
	D3D12UnorderedAccessView UAV;

	std::vector<MeshRenderer*> MeshRenderers;
	std::vector<Hlsl::Mesh>	   HlslMeshes;

	D3D12Buffer		Materials;
	Hlsl::Material* pMaterial = nullptr;
	D3D12Buffer		Lights;
	Hlsl::Light*	pLights = nullptr;
	D3D12Buffer		Meshes;
	Hlsl::Mesh*		pMeshes = nullptr;

	UINT NumMaterials = 0, NumLights = 0, NumMeshes = 0;

	RenderResourceHandle Viewport;

	int ViewMode = 0;
};

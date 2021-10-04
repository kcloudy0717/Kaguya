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
	struct IndirectCommand
	{
		D3D12_DRAW_ARGUMENTS DrawArguments;
	};

	static constexpr UINT64 TotalCommandBufferSizeInBytes = World::InstanceLimit * sizeof(IndirectCommand);
	static constexpr UINT64 CommandBufferCounterOffset =
		AlignUp(TotalCommandBufferSizeInBytes, D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT);

	D3D12CommandSignature CommandSignature;

	D3D12Buffer IndirectCommandBuffer;

	std::vector<MeshRenderer*> MeshRenderers;
	std::vector<Hlsl::Mesh>	   HlslMeshes;

	D3D12Buffer		Materials;
	Hlsl::Material* pMaterial = nullptr;
	D3D12Buffer		Lights;
	Hlsl::Light*	pLights = nullptr;
	D3D12Buffer		Meshes;
	Hlsl::Mesh*		pMeshes = nullptr;

	// Temp
	D3D12Buffer VisibilityBuffer;

	UINT NumMaterials = 0, NumLights = 0, NumMeshes = 0;

	RenderResourceHandle Viewport;

	int ViewMode = 0;
};

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
	D3D12RenderPass GBufferRenderPass;

	std::vector<MeshRenderer*> MeshRenderers;
	std::vector<HLSL::Mesh>	   HlslMeshes;

	D3D12Buffer		Materials;
	HLSL::Material* pMaterial = nullptr;
	D3D12Buffer		Lights;
	HLSL::Light*	pLights = nullptr;
	D3D12Buffer		Meshes;
	HLSL::Mesh*		pMeshes = nullptr;

	UINT NumMaterials = 0, NumLights = 0, NumMeshes = 0;

	RenderResourceHandle Viewport;

	int ViewMode = 0;
};

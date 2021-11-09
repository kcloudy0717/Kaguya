#include "PathIntegratorDXR1_1.h"
#include "RendererRegistry.h"

void PathIntegratorDXR1_1::SetViewportResolution(uint32_t Width, uint32_t Height)
{
	Resolution.RefreshViewportResolution(Width, Height);
	Resolution.RefreshRenderResolution(Width, Height);
}

void PathIntegratorDXR1_1::Initialize()
{
	Shaders::Compile();
	Libraries::Compile();
	RenderPasses::Compile();
	RootSignatures::Compile(RenderDevice);
	PipelineStates::Compile(RenderDevice);
	RaytracingPipelineStates::Compile(RenderDevice);

	AccelerationStructure = RaytracingAccelerationStructure(1, World::MeshLimit);
	AccelerationStructure.Initialize();

	Materials = D3D12Buffer(
		RenderCore::Device->GetDevice(),
		sizeof(Hlsl::Material) * World::MaterialLimit,
		sizeof(Hlsl::Material),
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_RESOURCE_FLAG_NONE);
	Materials.Initialize();
	pMaterial = Materials.GetCpuVirtualAddress<Hlsl::Material>();

	Lights = D3D12Buffer(
		RenderCore::Device->GetDevice(),
		sizeof(Hlsl::Light) * World::LightLimit,
		sizeof(Hlsl::Light),
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_RESOURCE_FLAG_NONE);
	Lights.Initialize();
	pLights = Lights.GetCpuVirtualAddress<Hlsl::Light>();
}

void PathIntegratorDXR1_1::Destroy()
{
}

void PathIntegratorDXR1_1::Render(World* World, D3D12CommandContext& Context)
{
}

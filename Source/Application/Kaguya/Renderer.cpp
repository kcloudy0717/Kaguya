#include "Renderer.h"
#include "RendererRegistry.h"

Renderer::Renderer(
	RHI::D3D12Device* Device,
	ShaderCompiler*	  Compiler)
	: Device(Device)
	, Compiler(Compiler)
	, Allocator(65536)
{
}

Renderer::~Renderer()
{
	Device->WaitIdle();
}

void Renderer::OnRenderOptions()
{
	RenderOptions();
}

void Renderer::OnRender(RHI::D3D12CommandContext& Context, World* World, WorldRenderView* WorldRenderView)
{
	D3D12ScopedEvent(Context, "Render");
	this->Render(World, WorldRenderView, Context);
	++FrameIndex;
}

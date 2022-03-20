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
	/*static bool CaptureOnce = false;
	if (!CaptureOnce)
	{
		RenderCore::Device->BeginCapture(Application::ExecutableDirectory / "GPU 0.wpix");
	}*/

	D3D12ScopedEvent(Context, "Render");
	this->Render(World, WorldRenderView, Context);

	++FrameIndex;
	/*if (!CaptureOnce)
	{
		CaptureOnce = true;
		RenderCore::Device->EndCapture();
	}*/
}

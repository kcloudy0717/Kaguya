#pragma once
#include "RenderGraph/RenderGraph.h"
#include "World/World.h"

class RendererPresent : public IPresent
{
public:
	RendererPresent(D3D12CommandContext& Context)
		: Context(Context)
	{
	}

	void PrePresent() override { SyncHandle = Context.Execute(false); }

	void PostPresent() override { SyncHandle.WaitForCompletion(); }

	D3D12CommandContext& Context;
	D3D12SyncHandle		 SyncHandle;
};

class Renderer
{
public:
	Renderer(HWND HWnd);
	virtual ~Renderer() = default;

	void OnSetViewportResolution(uint32_t Width, uint32_t Height);

	void OnInitialize();

	void OnDestroy();

	void OnRender(World* World);

	void OnResize(uint32_t Width, uint32_t Height);

	[[nodiscard]] void* GetViewportDescriptor() const noexcept;

protected:
	virtual void SetViewportResolution(uint32_t Width, uint32_t Height) = 0;
	virtual void Initialize()											= 0;
	virtual void Destroy()												= 0;
	virtual void Render(World* World, D3D12CommandContext& Context)		= 0;

protected:
	D3D12SwapChain SwapChain;

	RenderDevice		  RenderDevice;
	RenderGraphAllocator  Allocator;
	RenderGraphScheduler  Scheduler;
	RenderGraphRegistry	  Registry;
	RenderGraphResolution Resolution = {};

	size_t FrameIndex = 0;

	bool  ValidViewport = false;
	void* Viewport		= nullptr;
};

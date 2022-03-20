#pragma once
#include "RHI/RHI.h"
#include "RHI/RenderGraph/RenderGraph.h"
#include "Core/World/World.h"
#include "Core/Window.h"
#include "View.h"

#include "Globals.h"

class RendererPresent : public RHI::IPresent
{
public:
	RendererPresent(RHI::D3D12CommandContext& Context) noexcept
		: Context(Context)
	{
	}

	void PrePresent() override
	{
		SyncHandle = Context.Execute(false);
	}

	void PostPresent() override
	{
		SyncHandle.WaitForCompletion();
	}

	RHI::D3D12CommandContext& Context;
	RHI::D3D12SyncHandle	  SyncHandle;
};

class Renderer
{
public:
	Renderer(
		RHI::D3D12Device*	 Device,
		ShaderCompiler*		 Compiler);
	virtual ~Renderer();

	void OnRenderOptions();

	void OnRender(RHI::D3D12CommandContext& Context, World* World, WorldRenderView* WorldRenderView);

	void* GetViewportPtr() const { return Viewport; }

protected:
	virtual void RenderOptions()																		   = 0;
	virtual void Render(World* World, WorldRenderView* WorldRenderView, RHI::D3D12CommandContext& Context) = 0;

protected:
	RHI::D3D12Device*	 Device		= nullptr;
	ShaderCompiler*		 Compiler	= nullptr;

	RHI::RenderGraphAllocator Allocator;
	RHI::RenderGraphRegistry  Registry;

	size_t FrameIndex = 0;

	void* Viewport = nullptr;
};

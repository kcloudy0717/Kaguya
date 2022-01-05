#pragma once
#include "RenderGraph/RenderGraph.h"
#include "World/World.h"
#include "View.h"

#include "Graphics/UI/WorldWindow.h"
#include "Graphics/UI/InspectorWindow.h"
#include "Graphics/UI/AssetWindow.h"
#include "Graphics/UI/ConsoleWindow.h"

class RendererPresent : public IPresent
{
public:
	RendererPresent(D3D12CommandContext& Context)
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

	D3D12CommandContext& Context;
	D3D12SyncHandle		 SyncHandle;
};

class Renderer
{
public:
	Renderer(Window* MainWindow);
	virtual ~Renderer() = default;

	void OnInitialize();

	void OnDestroy();

	void OnRender(World* World);

	void OnResize(uint32_t Width, uint32_t Height);

protected:
	virtual void Initialize()										= 0;
	virtual void Destroy()											= 0;
	virtual void Render(World* World, D3D12CommandContext& Context) = 0;

protected:
	Window*		   MainWindow = nullptr;
	D3D12SwapChain SwapChain;

	RenderGraphAllocator Allocator;
	RenderGraphRegistry	 Registry;

	View View;

	size_t FrameIndex = 0;

	void* Viewport = nullptr;

	WorldWindow		WorldWindow;
	InspectorWindow InspectorWindow;
	AssetWindow		AssetWindow;
	ConsoleWindow	ConsoleWindow;
};

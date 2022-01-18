#pragma once
#include "RHI/RHI.h"
#include "RHI/RenderGraph/RenderGraph.h"
#include "Core/World/World.h"
#include "Core/Window.h"
#include "View.h"

#include "UI/WorldWindow.h"
#include "UI/InspectorWindow.h"
#include "UI/AssetWindow.h"

class RendererPresent : public IPresent
{
public:
	RendererPresent(D3D12CommandContext& Context) noexcept
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
	Renderer(D3D12Device* Device, ShaderCompiler* Compiler, Window* MainWindow);
	virtual ~Renderer() = default;

	void OnInitialize();

	void OnDestroy();

	void OnRender(World* World);

	void OnResize(uint32_t Width, uint32_t Height);

	void OnMove(std::int32_t x, std::int32_t y);

protected:
	virtual void Initialize()										= 0;
	virtual void Destroy()											= 0;
	virtual void Render(World* World, D3D12CommandContext& Context) = 0;

protected:
	D3D12Device*	Device	   = nullptr;
	ShaderCompiler* Compiler   = nullptr;
	Window*			MainWindow = nullptr;
	D3D12SwapChain	SwapChain;

	RenderGraphAllocator Allocator;
	RenderGraphRegistry	 Registry;

	View View;

	size_t FrameIndex = 0;

	void* Viewport = nullptr;

	WorldWindow		WorldWindow;
	InspectorWindow InspectorWindow;
	AssetWindow		AssetWindow;
};

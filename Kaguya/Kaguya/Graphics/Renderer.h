#pragma once
#include "RaytracingAccelerationStructure.h"
#include "RenderGraph/RenderGraph.h"
#include "World/World.h"

class Renderer
{
public:
	Renderer(World* pWorld)
		: pWorld(pWorld)
		, Allocator(64 * 1024)
		, Registry(Scheduler)
	{
	}
	virtual ~Renderer() = default;

	void OnSetViewportResolution(uint32_t Width, uint32_t Height);

	void OnInitialize();

	void OnRender();

	void OnResize(uint32_t Width, uint32_t Height);

	void OnDestroy();

	virtual void* GetViewportDescriptor() = 0;

protected:
	virtual void SetViewportResolution(uint32_t Width, uint32_t Height) = 0;
	virtual void Initialize()											= 0;
	virtual void Render(D3D12CommandContext& Context)					= 0;

protected:
	World* pWorld;

	RenderDevice		  RenderDevice;
	RenderGraphAllocator  Allocator;
	RenderGraphScheduler  Scheduler;
	RenderGraphRegistry	  Registry;
	RenderGraphResolution Resolution = {};

	size_t FrameIndex = 0;

	bool ValidViewport = false;
};

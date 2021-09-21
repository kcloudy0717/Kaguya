#pragma once
#include "RaytracingAccelerationStructure.h"
#include "RenderGraph/RenderGraph.h"
#include "World/World.h"

class Renderer
{
public:
	Renderer(World* pWorld)
		: pWorld(pWorld)
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

	UINT ViewportWidth = 0, ViewportHeight = 0;
	UINT RenderWidth = 0, RenderHeight = 0;

	RenderDevice RenderDevice;
	RenderGraph	 RenderGraph;
};

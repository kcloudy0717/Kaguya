#pragma once
#include "RaytracingAccelerationStructure.h"
#include "PathIntegrator.h"
#include "ToneMapper.h"
#include "FSRFilter.h"
#include "RenderGraph/RenderGraph.h"

class Renderer
{
public:
	Renderer(World& World)
		: World(World)
	{
	}

	void SetViewportMousePosition(float MouseX, float MouseY);

	void SetViewportResolution(uint32_t Width, uint32_t Height);

	void* GetViewportDescriptor();

	void OnInitialize();

	void OnRender();

	void OnResize(uint32_t Width, uint32_t Height);

	void OnDestroy();

private:
	World& World;

	float ViewportMouseX = 0.0f, ViewportMouseY = 0.0f;
	UINT  ViewportWidth = 0, ViewportHeight = 0;
	UINT  RenderWidth = 0, RenderHeight = 0;

	D3D12_VIEWPORT Viewport	   = {};
	D3D12_RECT	   ScissorRect = {};

	RaytracingAccelerationStructure AccelerationStructure;
	// PathIntegrator_DXR_1_0			PathIntegrator;
	// ToneMapper						ToneMapper;
	// FSRFilter						FSRFilter;

	RaytracingAccelerationStructureManager Manager;

	struct Settings
	{
		inline static UINT NumAccumulatedSamples = 0;
	};
	PathIntegratorState PathIntegratorState;
	FSRState			FSRState;

	Buffer			Materials;
	HLSL::Material* pMaterials = nullptr;
	Buffer			Lights;
	HLSL::Light*	pLights		 = nullptr;
	UINT			NumMaterials = 0, NumLights = 0;

	RenderGraph RenderGraph;
};

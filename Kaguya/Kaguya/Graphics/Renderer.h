#pragma once
#include "RaytracingAccelerationStructure.h"
#include "PathIntegrator.h"
#include "ToneMapper.h"
#include "FSRFilter.h"

class Renderer
{
public:
	Renderer();

	void SetViewportMousePosition(float MouseX, float MouseY);

	void SetViewportResolution(uint32_t Width, uint32_t Height);

	const ShaderResourceView& GetViewportDescriptor() { return m_FSRFilter.GetSRV(); }

	void OnInitialize();

	void OnRender(World& World);

	void OnResize(uint32_t Width, uint32_t Height);

	void OnDestroy();

	void RequestCapture();

private:
	float ViewportMouseX, ViewportMouseY;
	UINT  ViewportWidth, ViewportHeight;
	UINT  RenderWidth, RenderHeight;

	D3D12_VIEWPORT Viewport;
	D3D12_RECT	   ScissorRect;

	RaytracingAccelerationStructure AccelerationStructure;
	PathIntegrator_DXR_1_0			m_PathIntegrator;
	ToneMapper						m_ToneMapper;
	FSRFilter						m_FSRFilter;

	FSRState State;

	Buffer			Materials;
	HLSL::Material* pMaterials = nullptr;
	Buffer			Lights;
	HLSL::Light*	pLights = nullptr;
};

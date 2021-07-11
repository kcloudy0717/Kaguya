#pragma once
#include "RaytracingAccelerationStructure.h"
#include "PathIntegrator.h"
#include "ToneMapper.h"

class Renderer
{
public:
	Renderer();

	void SetViewportMousePosition(float MouseX, float MouseY);

	void SetViewportResolution(uint32_t Width, uint32_t Height);

	const ShaderResourceView& GetViewportDescriptor() { return m_ToneMapper.GetSRV(); }

	void OnInitialize();

	void OnRender(Scene& Scene);

	void OnResize(uint32_t Width, uint32_t Height);

	void OnDestroy();

	void RequestCapture();

private:
	uint32_t Width, Height;

	float		   ViewportMouseX, ViewportMouseY;
	uint32_t	   ViewportWidth, ViewportHeight;
	D3D12_VIEWPORT Viewport;
	D3D12_RECT	   ScissorRect;

	RaytracingAccelerationStructure AccelerationStructure;
	PathIntegrator					m_PathIntegrator;
	ToneMapper						m_ToneMapper;

	Buffer			Materials;
	HLSL::Material* pMaterials = nullptr;
	Buffer			Lights;
	HLSL::Light*	pLights = nullptr;
};

#pragma once
#include <Core/RenderSystem.h>

#include "RaytracingAccelerationStructure.h"
#include "PathIntegrator.h"
#include "ToneMapper.h"

class Renderer : public RenderSystem
{
public:
	Renderer();
	~Renderer() override;

	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;

	void SetViewportMousePosition(float MouseX, float MouseY);

	void SetViewportResolution(uint32_t Width, uint32_t Height);

	const ShaderResourceView& GetViewportDescriptor() { return m_ToneMapper.GetSRV(); }

protected:
	void Initialize() override;

	void Render(Scene& Scene) override;

	void Resize(uint32_t Width, uint32_t Height) override;

	void Destroy() override;

	void RequestCapture() override;

private:
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

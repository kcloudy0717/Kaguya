#pragma once
#include <Core/RenderSystem.h>

#include "RaytracingAccelerationStructure.h"
#include "PathIntegrator.h"
#include "Picking.h"
#include "ToneMapper.h"

class Renderer : public RenderSystem
{
public:
	Renderer();
	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;
	~Renderer() override;

	Descriptor GetViewportDescriptor();
	Entity GetSelectedEntity();

	void SetViewportMousePosition(float MouseX, float MouseY);
	void SetViewportResolution(uint32_t Width, uint32_t Height);
protected:
	void Initialize() override;
	void Render(const Time& Time, Scene& Scene) override;
	void Resize(uint32_t Width, uint32_t Height) override;
	void Destroy() override;
	void RequestCapture() override;
private:
	float m_ViewportMouseX, m_ViewportMouseY;
	uint32_t m_ViewportWidth, m_ViewportHeight;
	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_ScissorRect;

	RaytracingAccelerationStructure m_RaytracingAccelerationStructure;
	PathIntegrator m_PathIntegrator;
	Picking m_Picking;
	ToneMapper m_ToneMapper;

	std::shared_ptr<Resource> m_Materials;
	HLSL::Material* m_pMaterials = nullptr;
	std::shared_ptr<Resource> m_Lights;
	HLSL::Light* m_pLights = nullptr;
};
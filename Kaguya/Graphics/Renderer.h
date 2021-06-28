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
	~Renderer() override;

	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;

	void SetViewportMousePosition(float MouseX, float MouseY);

	void SetViewportResolution(uint32_t Width, uint32_t Height);

	const ShaderResourceView& GetViewportDescriptor() { return m_ToneMapper.GetSRV(); }
	Entity					  GetSelectedEntity() { return m_Picking.GetSelectedEntity().value_or(Entity()); }

protected:
	void Initialize() override;

	void Render(Scene& Scene) override;

	void Resize(uint32_t Width, uint32_t Height) override;

	void Destroy() override;

	void RequestCapture() override;

private:
	float		   m_ViewportMouseX, m_ViewportMouseY;
	uint32_t	   m_ViewportWidth, m_ViewportHeight;
	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT	   m_ScissorRect;

	RaytracingAccelerationStructure m_RaytracingAccelerationStructure;
	PathIntegrator					m_PathIntegrator;
	Picking							m_Picking;
	ToneMapper						m_ToneMapper;

	Buffer			m_Materials;
	HLSL::Material* m_pMaterials = nullptr;
	Buffer			m_Lights;
	HLSL::Light*	m_pLights = nullptr;
};

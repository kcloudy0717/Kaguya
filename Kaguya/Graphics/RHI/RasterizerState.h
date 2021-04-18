#pragma once
#include <d3d12.h>
#include "AbstractionLayer.h"

class RasterizerState
{
public:
	enum class FillMode
	{
		Wireframe,   // Draw lines connecting the vertices.
		Solid        // Fill the triangles formed by the vertices
	};

	enum class CullMode
	{
		None,	// Always draw all triangles
		Front,	// Do not draw triangles that are front-facing
		Back	// Do not draw triangles that are back-facing
	};

	RasterizerState();

	void SetFillMode(FillMode FillMode);

	void SetCullMode(CullMode CullMode);

	// Determines how to interpret triangle direction
	void SetFrontCounterClockwise(bool FrontCounterClockwise);

	void SetDepthBias(int DepthBias);

	void SetDepthBiasClamp(float DepthBiasClamp);

	void SetSlopeScaledDepthBias(float SlopeScaledDepthBias);

	void SetDepthClipEnable(bool DepthClipEnable);

	void SetMultisampleEnable(bool MultisampleEnable);

	void SetAntialiasedLineEnable(bool AntialiasedLineEnable);

	void SetForcedSampleCount(unsigned int ForcedSampleCount);

	void SetConservativeRaster(bool ConservativeRaster);

	operator D3D12_RASTERIZER_DESC() const noexcept;
private:
	FillMode		m_FillMode;
	CullMode		m_CullMode;
	bool			m_FrontCounterClockwise;
	int				m_DepthBias;
	float			m_DepthBiasClamp;
	float			m_SlopeScaledDepthBias;
	bool			m_DepthClipEnable;
	bool			m_MultisampleEnable;
	bool			m_AntialiasedLineEnable;
	unsigned int	m_ForcedSampleCount;
	bool			m_ConservativeRaster;
};

D3D12_FILL_MODE GetD3D12FillMode(RasterizerState::FillMode FillMode);
D3D12_CULL_MODE GetD3D12CullMode(RasterizerState::CullMode CullMode);
#include "pch.h"
#include "RasterizerState.h"
#include "D3D12/d3dx12.h"

RasterizerState::RasterizerState()
{
	SetFillMode(FillMode::Solid);
	SetCullMode(CullMode::Back);
	SetFrontCounterClockwise(false);
	SetDepthBias(0);
	SetDepthBiasClamp(0.0f);
	SetSlopeScaledDepthBias(0.0f);
	SetDepthClipEnable(true);
	SetMultisampleEnable(false);
	SetAntialiasedLineEnable(false);
	SetForcedSampleCount(0);
	SetConservativeRaster(false);
}

void RasterizerState::SetFillMode(FillMode FillMode)
{
	m_FillMode = FillMode;
}

void RasterizerState::SetCullMode(CullMode CullMode)
{
	m_CullMode = CullMode;
}

void RasterizerState::SetFrontCounterClockwise(bool FrontCounterClockwise)
{
	m_FrontCounterClockwise = FrontCounterClockwise;
}

void RasterizerState::SetDepthBias(int DepthBias)
{
	m_DepthBias = DepthBias;
}

void RasterizerState::SetDepthBiasClamp(float DepthBiasClamp)
{
	m_DepthBiasClamp = DepthBiasClamp;
}

void RasterizerState::SetSlopeScaledDepthBias(float SlopeScaledDepthBias)
{
	m_SlopeScaledDepthBias = SlopeScaledDepthBias;
}

void RasterizerState::SetDepthClipEnable(bool DepthClipEnable)
{
	m_DepthClipEnable = DepthClipEnable;
}

void RasterizerState::SetMultisampleEnable(bool MultisampleEnable)
{
	m_MultisampleEnable = MultisampleEnable;
}

void RasterizerState::SetAntialiasedLineEnable(bool AntialiasedLineEnable)
{
	m_AntialiasedLineEnable = AntialiasedLineEnable;
}

void RasterizerState::SetForcedSampleCount(unsigned int ForcedSampleCount)
{
	m_ForcedSampleCount = ForcedSampleCount;
}

void RasterizerState::SetConservativeRaster(bool ConservativeRaster)
{
	m_ConservativeRaster = ConservativeRaster;
}

RasterizerState::operator D3D12_RASTERIZER_DESC() const noexcept
{
	D3D12_RASTERIZER_DESC Desc	= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	{
		Desc.FillMode				= GetD3D12FillMode(m_FillMode);
		Desc.CullMode				= GetD3D12CullMode(m_CullMode);
		Desc.FrontCounterClockwise	= m_FrontCounterClockwise ? TRUE : FALSE;
		Desc.DepthBias				= m_DepthBias;
		Desc.DepthBiasClamp			= m_DepthBiasClamp;
		Desc.SlopeScaledDepthBias	= m_SlopeScaledDepthBias;
		Desc.DepthClipEnable		= m_DepthClipEnable ? TRUE : FALSE;
		Desc.MultisampleEnable		= m_MultisampleEnable ? TRUE : FALSE;
		Desc.AntialiasedLineEnable	= m_AntialiasedLineEnable ? TRUE : FALSE;
		Desc.ForcedSampleCount		= m_ForcedSampleCount;
		Desc.ConservativeRaster		= m_ConservativeRaster ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF : D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON;
	}
	return Desc;
}

D3D12_FILL_MODE GetD3D12FillMode(RasterizerState::FillMode FillMode)
{
	switch (FillMode)
	{
	case RasterizerState::FillMode::Wireframe:	return D3D12_FILL_MODE_WIREFRAME;
	case RasterizerState::FillMode::Solid:		return D3D12_FILL_MODE_SOLID;
	}
	return D3D12_FILL_MODE();
}

D3D12_CULL_MODE GetD3D12CullMode(RasterizerState::CullMode CullMode)
{
	switch (CullMode)
	{
	case RasterizerState::CullMode::None:	return D3D12_CULL_MODE_NONE;
	case RasterizerState::CullMode::Front:	return D3D12_CULL_MODE_FRONT;
	case RasterizerState::CullMode::Back:	return D3D12_CULL_MODE_BACK;
	}
	return D3D12_CULL_MODE();
}
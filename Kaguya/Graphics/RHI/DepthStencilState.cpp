#include "pch.h"
#include "DepthStencilState.h"
#include "D3D12/d3dx12.h"

DepthStencilState::Stencil::operator D3D12_DEPTH_STENCILOP_DESC() const noexcept
{
	D3D12_DEPTH_STENCILOP_DESC Desc = {};
	Desc.StencilFailOp				= GetD3D12StencilOp(StencilFailOp);
	Desc.StencilDepthFailOp			= GetD3D12StencilOp(StencilDepthFailOp);
	Desc.StencilPassOp				= GetD3D12StencilOp(StencilPassOp);
	Desc.StencilFunc				= GetD3D12ComparisonFunc(StencilFunc);
	return Desc;
}

DepthStencilState::DepthStencilState()
{
	// Default states https://docs.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_depth_stencil_desc#remarks
	SetDepthEnable(true);
	SetDepthWrite(true);
	SetDepthFunc(ComparisonFunc::Less);
	SetStencilEnable(false);
	SetStencilReadMask(0xff);
	SetStencilWriteMask(0xff);
}

void DepthStencilState::SetDepthEnable(bool DepthEnable)
{
	m_DepthEnable = DepthEnable;
}

void DepthStencilState::SetDepthWrite(bool DepthWrite)
{
	m_DepthWrite = DepthWrite;
}

void DepthStencilState::SetDepthFunc(ComparisonFunc DepthFunc)
{
	m_DepthFunc = DepthFunc;
}

void DepthStencilState::SetStencilEnable(bool StencilEnable)
{
	m_StencilEnable = StencilEnable;
}

void DepthStencilState::SetStencilReadMask(UINT8 StencilReadMask)
{
	m_StencilReadMask = StencilReadMask;
}

void DepthStencilState::SetStencilWriteMask(UINT8 StencilWriteMask)
{
	m_StencilWriteMask = StencilWriteMask;
}

void DepthStencilState::SetStencilOp
(
	Face Face,
	StencilOperation StencilFailOp,
	StencilOperation StencilDepthFailOp,
	StencilOperation StencilPassOp
)
{
	if (Face == Face::FrontAndBack)
	{
		SetStencilOp(Face::Front, StencilFailOp, StencilDepthFailOp, StencilPassOp);
		SetStencilOp(Face::Back, StencilFailOp, StencilDepthFailOp, StencilPassOp);
	}
	Stencil& Stencil			= Face == Face::Front ? m_FrontFace : m_BackFace;
	Stencil.StencilFailOp		= StencilFailOp;
	Stencil.StencilDepthFailOp	= StencilDepthFailOp;
	Stencil.StencilPassOp		= StencilPassOp;
}

void DepthStencilState::SetStencilFunc(Face Face, ComparisonFunc StencilFunc)
{
	if (Face == Face::FrontAndBack)
	{
		SetStencilFunc(Face::Front, StencilFunc);
		SetStencilFunc(Face::Back, StencilFunc);
	}
	Stencil& Stencil	= Face == Face::Front ? m_FrontFace : m_BackFace;
	Stencil.StencilFunc = StencilFunc;
}

DepthStencilState::operator D3D12_DEPTH_STENCIL_DESC() const noexcept
{
	D3D12_DEPTH_STENCIL_DESC Desc	= {};
	Desc.DepthEnable				= m_DepthEnable ? TRUE : FALSE;
	Desc.DepthWriteMask				= m_DepthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
	Desc.DepthFunc					= GetD3D12ComparisonFunc(m_DepthFunc);
	Desc.StencilEnable				= m_StencilEnable ? TRUE : FALSE;
	Desc.StencilReadMask			= m_StencilReadMask;
	Desc.StencilWriteMask			= m_StencilWriteMask;
	Desc.FrontFace					= m_FrontFace;
	Desc.BackFace					= m_BackFace;
	return Desc;
}

D3D12_STENCIL_OP GetD3D12StencilOp(DepthStencilState::StencilOperation Op)
{
	switch (Op)
	{
	case DepthStencilState::StencilOperation::Keep:				return D3D12_STENCIL_OP_KEEP;
	case DepthStencilState::StencilOperation::Zero:				return D3D12_STENCIL_OP_ZERO;
	case DepthStencilState::StencilOperation::Replace:			return D3D12_STENCIL_OP_REPLACE;
	case DepthStencilState::StencilOperation::IncreaseSaturate: return D3D12_STENCIL_OP_INCR_SAT;
	case DepthStencilState::StencilOperation::DecreaseSaturate: return D3D12_STENCIL_OP_DECR_SAT;
	case DepthStencilState::StencilOperation::Invert:			return D3D12_STENCIL_OP_INVERT;
	case DepthStencilState::StencilOperation::Increase:			return D3D12_STENCIL_OP_INCR;
	case DepthStencilState::StencilOperation::Decrease:			return D3D12_STENCIL_OP_DECR;
	}
	return D3D12_STENCIL_OP();
}
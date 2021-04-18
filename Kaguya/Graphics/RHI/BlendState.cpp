#include "pch.h"
#include "BlendState.h"
#include "D3D12/d3dx12.h"

BlendState::RenderTarget::operator D3D12_RENDER_TARGET_BLEND_DESC() const noexcept
{
	D3D12_RENDER_TARGET_BLEND_DESC Desc = {};
	Desc.BlendEnable					= Operation == Operation::Blend ? TRUE : FALSE;
	Desc.LogicOpEnable					= Operation == Operation::Logic ? TRUE : FALSE;
	Desc.SrcBlend						= GetD3D12Blend(SrcBlendRGB);
	Desc.DestBlend						= GetD3D12Blend(DstBlendRGB);
	Desc.BlendOp						= GetD3D12BlendOp(BlendOpRGB);
	Desc.SrcBlendAlpha					= GetD3D12Blend(SrcBlendAlpha);
	Desc.DestBlendAlpha					= GetD3D12Blend(DstBlendAlpha);
	Desc.BlendOpAlpha					= GetD3D12BlendOp(BlendOpAlpha);
	Desc.LogicOp						= GetD3D12LogicOp(LogicOpRGB);
	Desc.RenderTargetWriteMask			|= WriteMask.WriteRed ? D3D12_COLOR_WRITE_ENABLE_RED : 0;
	Desc.RenderTargetWriteMask			|= WriteMask.WriteGreen ? D3D12_COLOR_WRITE_ENABLE_GREEN : 0;
	Desc.RenderTargetWriteMask			|= WriteMask.WriteBlue ? D3D12_COLOR_WRITE_ENABLE_BLUE : 0;
	Desc.RenderTargetWriteMask			|= WriteMask.WriteAlpha ? D3D12_COLOR_WRITE_ENABLE_ALPHA : 0;
	return Desc;
}

BlendState::BlendState()
{
	SetAlphaToCoverageEnable(false);
	SetIndependentBlendEnable(false);
	m_NumRenderTargets = 0;
}

void BlendState::SetAlphaToCoverageEnable(bool AlphaToCoverageEnable)
{
	m_AlphaToCoverageEnable = AlphaToCoverageEnable;
}

void BlendState::SetIndependentBlendEnable(bool IndependentBlendEnable)
{
	m_IndependentBlendEnable = IndependentBlendEnable;
}

void BlendState::AddRenderTargetForBlendOp(Factor SrcBlendRGB, Factor DstBlendRGB, BlendOp BlendOpRGB, Factor SrcBlendAlpha, Factor DstBlendAlpha, BlendOp BlendOpAlpha)
{
	if (m_NumRenderTargets >= 8)
		return;

	RenderTarget RenderTarget				= {};
	RenderTarget.Operation					= Operation::Blend;
	RenderTarget.SrcBlendRGB				= SrcBlendRGB;
	RenderTarget.DstBlendRGB				= DstBlendAlpha;
	RenderTarget.BlendOpRGB					= BlendOpRGB;
	RenderTarget.SrcBlendAlpha				= SrcBlendRGB;
	RenderTarget.DstBlendAlpha				= DstBlendAlpha;
	RenderTarget.BlendOpAlpha				= BlendOpRGB;
	m_RenderTargets[m_NumRenderTargets++]	= RenderTarget;
}

void BlendState::AddRenderTargetForLogicOp(LogicOp LogicOpRGB)
{
	if (m_NumRenderTargets >= 8)
		return;

	RenderTarget RenderTarget				= {};
	RenderTarget.Operation					= Operation::Logic;
	RenderTarget.LogicOpRGB					= LogicOpRGB;
	m_RenderTargets[m_NumRenderTargets++]	= RenderTarget;
}

BlendState::operator D3D12_BLEND_DESC() const noexcept
{
	D3D12_BLEND_DESC Desc		= CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	Desc.AlphaToCoverageEnable	= m_AlphaToCoverageEnable ? TRUE : FALSE;
	Desc.IndependentBlendEnable = m_IndependentBlendEnable ? TRUE : FALSE;
	for (UINT i = 0; i < m_NumRenderTargets; ++i)
	{
		Desc.RenderTarget[i]	= m_RenderTargets[i];
	}
	return Desc;
}

D3D12_BLEND_OP GetD3D12BlendOp(BlendState::BlendOp Op)
{
	switch (Op)
	{
	case BlendState::BlendOp::Add:				return D3D12_BLEND_OP_ADD;
	case BlendState::BlendOp::Subtract:			return D3D12_BLEND_OP_SUBTRACT;
	case BlendState::BlendOp::ReverseSubstract: return D3D12_BLEND_OP_REV_SUBTRACT;
	case BlendState::BlendOp::Min:				return D3D12_BLEND_OP_MIN;
	case BlendState::BlendOp::Max:				return D3D12_BLEND_OP_MAX;
	}
	return D3D12_BLEND_OP();
}

D3D12_LOGIC_OP GetD3D12LogicOp(BlendState::LogicOp Op)
{
	switch (Op)
	{
	case BlendState::LogicOp::Clear:		return D3D12_LOGIC_OP_CLEAR;
	case BlendState::LogicOp::Set:			return D3D12_LOGIC_OP_SET;
	case BlendState::LogicOp::Copy:			return D3D12_LOGIC_OP_COPY;
	case BlendState::LogicOp::CopyInverted: return D3D12_LOGIC_OP_COPY_INVERTED;
	case BlendState::LogicOp::NoOperation:	return D3D12_LOGIC_OP_NOOP;
	case BlendState::LogicOp::Invert:		return D3D12_LOGIC_OP_INVERT;
	case BlendState::LogicOp::And:			return D3D12_LOGIC_OP_AND;
	case BlendState::LogicOp::Nand:			return D3D12_LOGIC_OP_NAND;
	case BlendState::LogicOp::Or:			return D3D12_LOGIC_OP_OR;
	case BlendState::LogicOp::Nor:			return D3D12_LOGIC_OP_NOR;
	case BlendState::LogicOp::Xor:			return D3D12_LOGIC_OP_XOR;
	case BlendState::LogicOp::Equivalence:	return D3D12_LOGIC_OP_EQUIV;
	case BlendState::LogicOp::AndReverse:	return D3D12_LOGIC_OP_AND_REVERSE;
	case BlendState::LogicOp::AndInverted:	return D3D12_LOGIC_OP_AND_INVERTED;
	case BlendState::LogicOp::OrReverse:	return D3D12_LOGIC_OP_OR_REVERSE;
	case BlendState::LogicOp::OrInverted:	return D3D12_LOGIC_OP_OR_INVERTED;
	}
	return D3D12_LOGIC_OP();
}

D3D12_BLEND GetD3D12Blend(BlendState::Factor Factor)
{
	switch (Factor)
	{
	case BlendState::Factor::Zero:					return D3D12_BLEND_ZERO;
	case BlendState::Factor::One:					return D3D12_BLEND_ONE;
	case BlendState::Factor::SrcColor:				return D3D12_BLEND_SRC_COLOR;
	case BlendState::Factor::OneMinusSrcColor:		return D3D12_BLEND_INV_SRC_COLOR;
	case BlendState::Factor::DstColor:				return D3D12_BLEND_DEST_COLOR;
	case BlendState::Factor::OneMinusDstColor:		return D3D12_BLEND_INV_DEST_COLOR;
	case BlendState::Factor::SrcAlpha:				return D3D12_BLEND_SRC_ALPHA;
	case BlendState::Factor::OneMinusSrcAlpha:		return D3D12_BLEND_INV_SRC_ALPHA;
	case BlendState::Factor::DstAlpha:				return D3D12_BLEND_INV_DEST_ALPHA;
	case BlendState::Factor::OneMinusDstAlpha:		return D3D12_BLEND_INV_DEST_ALPHA;
	case BlendState::Factor::BlendFactor:			return D3D12_BLEND_BLEND_FACTOR;
	case BlendState::Factor::OneMinusBlendFactor:	return D3D12_BLEND_INV_BLEND_FACTOR;
	case BlendState::Factor::SrcAlphaSaturate:		return D3D12_BLEND_SRC_ALPHA_SAT;
	case BlendState::Factor::Src1Color:				return D3D12_BLEND_SRC1_COLOR;
	case BlendState::Factor::OneMinusSrc1Color:		return D3D12_BLEND_INV_SRC1_COLOR;
	case BlendState::Factor::Src1Alpha:				return D3D12_BLEND_SRC1_ALPHA;
	case BlendState::Factor::OneMinusSrc1Alpha:		return D3D12_BLEND_INV_SRC1_ALPHA;
	}
	return D3D12_BLEND();
}
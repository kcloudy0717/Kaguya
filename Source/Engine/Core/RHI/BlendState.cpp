#include "BlendState.h"

BlendState::operator D3D12_BLEND_DESC() const noexcept
{
	D3D12_BLEND_DESC Desc		= CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	Desc.AlphaToCoverageEnable	= AlphaToCoverageEnable ? TRUE : FALSE;
	Desc.IndependentBlendEnable = IndependentBlendEnable ? TRUE : FALSE;
	for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
	{
		Desc.RenderTarget[i] = RenderTargets[i];
	}
	return Desc;
}

BlendState::RenderTarget::operator D3D12_RENDER_TARGET_BLEND_DESC() const noexcept
{
	D3D12_RENDER_TARGET_BLEND_DESC Desc = {};
	Desc.BlendEnable					= BlendEnable ? TRUE : FALSE;
	Desc.LogicOpEnable					= FALSE;
	Desc.SrcBlend						= ToD3D12Blend(SrcBlendRgb);
	Desc.DestBlend						= ToD3D12Blend(DstBlendRgb);
	Desc.BlendOp						= ToD3D12BlendOp(BlendOpRgb);
	Desc.SrcBlendAlpha					= ToD3D12Blend(SrcBlendAlpha);
	Desc.DestBlendAlpha					= ToD3D12Blend(DstBlendAlpha);
	Desc.BlendOpAlpha					= ToD3D12BlendOp(BlendOpAlpha);
	Desc.LogicOp						= D3D12_LOGIC_OP_CLEAR;
	Desc.RenderTargetWriteMask |= WriteMask.R ? D3D12_COLOR_WRITE_ENABLE_RED : 0;
	Desc.RenderTargetWriteMask |= WriteMask.G ? D3D12_COLOR_WRITE_ENABLE_GREEN : 0;
	Desc.RenderTargetWriteMask |= WriteMask.B ? D3D12_COLOR_WRITE_ENABLE_BLUE : 0;
	Desc.RenderTargetWriteMask |= WriteMask.A ? D3D12_COLOR_WRITE_ENABLE_ALPHA : 0;
	return Desc;
}

BlendState::BlendState()
{
	SetAlphaToCoverageEnable(false);
	SetIndependentBlendEnable(false);
}

void BlendState::SetAlphaToCoverageEnable(bool AlphaToCoverageEnable)
{
	this->AlphaToCoverageEnable = AlphaToCoverageEnable;
}

void BlendState::SetIndependentBlendEnable(bool IndependentBlendEnable)
{
	this->IndependentBlendEnable = IndependentBlendEnable;
}

void BlendState::SetRenderTargetBlendDesc(
	UINT	 Index,
	FACTOR	 SrcBlendRgb,
	FACTOR	 DstBlendRgb,
	BLEND_OP BlendOpRgb,
	FACTOR	 SrcBlendAlpha,
	FACTOR	 DstBlendAlpha,
	BLEND_OP BlendOpAlpha)
{
	RenderTarget RenderTarget  = {};
	RenderTarget.BlendEnable   = true;
	RenderTarget.SrcBlendRgb   = SrcBlendRgb;
	RenderTarget.DstBlendRgb   = DstBlendRgb;
	RenderTarget.BlendOpRgb	   = BlendOpRgb;
	RenderTarget.SrcBlendAlpha = SrcBlendAlpha;
	RenderTarget.DstBlendAlpha = DstBlendAlpha;
	RenderTarget.BlendOpAlpha  = BlendOpAlpha;
	RenderTargets[Index]	   = RenderTarget;
}

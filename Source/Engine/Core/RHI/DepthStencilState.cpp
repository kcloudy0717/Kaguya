#include "DepthStencilState.h"

DepthStencilState::Stencil::operator D3D12_DEPTH_STENCILOP_DESC() const noexcept
{
	D3D12_DEPTH_STENCILOP_DESC Desc = {};
	Desc.StencilFailOp				= ToD3D12StencilOp(FailOp);
	Desc.StencilDepthFailOp			= ToD3D12StencilOp(DepthFailOp);
	Desc.StencilPassOp				= ToD3D12StencilOp(PassOp);
	Desc.StencilFunc				= ToD3D12ComparisonFunc(Func);
	return Desc;
}

DepthStencilState::operator D3D12_DEPTH_STENCIL_DESC() const noexcept
{
	D3D12_DEPTH_STENCIL_DESC Desc = {};
	Desc.DepthEnable			  = DepthEnable ? TRUE : FALSE;
	Desc.DepthWriteMask			  = DepthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
	Desc.DepthFunc				  = ToD3D12ComparisonFunc(DepthFunc);
	Desc.StencilEnable			  = StencilEnable ? TRUE : FALSE;
	Desc.StencilReadMask		  = StencilReadMask;
	Desc.StencilWriteMask		  = StencilWriteMask;
	Desc.FrontFace				  = FrontFace;
	Desc.BackFace				  = BackFace;
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
	this->DepthEnable = DepthEnable;
}

void DepthStencilState::SetDepthWrite(bool DepthWrite)
{
	this->DepthWrite = DepthWrite;
}

void DepthStencilState::SetDepthFunc(ComparisonFunc DepthFunc)
{
	this->DepthFunc = DepthFunc;
}

void DepthStencilState::SetStencilEnable(bool StencilEnable)
{
	this->StencilEnable = StencilEnable;
}

void DepthStencilState::SetStencilReadMask(UINT8 StencilReadMask)
{
	this->StencilReadMask = StencilReadMask;
}

void DepthStencilState::SetStencilWriteMask(UINT8 StencilWriteMask)
{
	this->StencilWriteMask = StencilWriteMask;
}

void DepthStencilState::SetStencilOp(
	EFace	   Face,
	EStencilOp StencilFailOp,
	EStencilOp StencilDepthFailOp,
	EStencilOp StencilPassOp)
{
	if (Face == EFace::FrontAndBack)
	{
		SetStencilOp(EFace::Front, StencilFailOp, StencilDepthFailOp, StencilPassOp);
		SetStencilOp(EFace::Back, StencilFailOp, StencilDepthFailOp, StencilPassOp);
	}
	Stencil& Stencil	= Face == EFace::Front ? FrontFace : BackFace;
	Stencil.FailOp		= StencilFailOp;
	Stencil.DepthFailOp = StencilDepthFailOp;
	Stencil.PassOp		= StencilPassOp;
}

void DepthStencilState::SetStencilFunc(EFace Face, ComparisonFunc StencilFunc)
{
	if (Face == EFace::FrontAndBack)
	{
		SetStencilFunc(EFace::Front, StencilFunc);
		SetStencilFunc(EFace::Back, StencilFunc);
	}
	Stencil& Stencil = Face == EFace::Front ? FrontFace : BackFace;
	Stencil.Func	 = StencilFunc;
}

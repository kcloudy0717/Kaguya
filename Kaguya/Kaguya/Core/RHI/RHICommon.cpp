#include "RHICommon.h"

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
	UINT	Index,
	Factor	SrcBlendRGB,
	Factor	DstBlendRGB,
	BlendOp BlendOpRGB,
	Factor	SrcBlendAlpha,
	Factor	DstBlendAlpha,
	BlendOp BlendOpAlpha)
{
	RenderTarget RenderTarget  = {};
	RenderTarget.BlendEnable   = true;
	RenderTarget.SrcBlendRGB   = SrcBlendRGB;
	RenderTarget.DstBlendRGB   = DstBlendRGB;
	RenderTarget.BlendOpRGB	   = BlendOpRGB;
	RenderTarget.SrcBlendAlpha = SrcBlendAlpha;
	RenderTarget.DstBlendAlpha = DstBlendAlpha;
	RenderTarget.BlendOpAlpha  = BlendOpAlpha;
	RenderTargets[Index]	   = RenderTarget;
}

RasterizerState::RasterizerState()
{
	SetFillMode(EFillMode::Solid);
	SetCullMode(ECullMode::Back);
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

void RasterizerState::SetFillMode(EFillMode FillMode)
{
	this->FillMode = FillMode;
}

void RasterizerState::SetCullMode(ECullMode CullMode)
{
	this->CullMode = CullMode;
}

void RasterizerState::SetFrontCounterClockwise(bool FrontCounterClockwise)
{
	this->FrontCounterClockwise = FrontCounterClockwise;
}

void RasterizerState::SetDepthBias(int DepthBias)
{
	this->DepthBias = DepthBias;
}

void RasterizerState::SetDepthBiasClamp(float DepthBiasClamp)
{
	this->DepthBiasClamp = DepthBiasClamp;
}

void RasterizerState::SetSlopeScaledDepthBias(float SlopeScaledDepthBias)
{
	this->SlopeScaledDepthBias = SlopeScaledDepthBias;
}

void RasterizerState::SetDepthClipEnable(bool DepthClipEnable)
{
	this->DepthClipEnable = DepthClipEnable;
}

void RasterizerState::SetMultisampleEnable(bool MultisampleEnable)
{
	this->MultisampleEnable = MultisampleEnable;
}

void RasterizerState::SetAntialiasedLineEnable(bool AntialiasedLineEnable)
{
	this->AntialiasedLineEnable = AntialiasedLineEnable;
}

void RasterizerState::SetForcedSampleCount(unsigned int ForcedSampleCount)
{
	this->ForcedSampleCount = ForcedSampleCount;
}

void RasterizerState::SetConservativeRaster(bool ConservativeRaster)
{
	this->ConservativeRaster = ConservativeRaster;
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
	Face			 Face,
	StencilOperation StencilFailOp,
	StencilOperation StencilDepthFailOp,
	StencilOperation StencilPassOp)
{
	if (Face == Face::FrontAndBack)
	{
		SetStencilOp(Face::Front, StencilFailOp, StencilDepthFailOp, StencilPassOp);
		SetStencilOp(Face::Back, StencilFailOp, StencilDepthFailOp, StencilPassOp);
	}
	Stencil& Stencil		   = Face == Face::Front ? FrontFace : BackFace;
	Stencil.StencilFailOp	   = StencilFailOp;
	Stencil.StencilDepthFailOp = StencilDepthFailOp;
	Stencil.StencilPassOp	   = StencilPassOp;
}

void DepthStencilState::SetStencilFunc(Face Face, ComparisonFunc StencilFunc)
{
	if (Face == Face::FrontAndBack)
	{
		SetStencilFunc(Face::Front, StencilFunc);
		SetStencilFunc(Face::Back, StencilFunc);
	}
	Stencil& Stencil	= Face == Face::Front ? FrontFace : BackFace;
	Stencil.StencilFunc = StencilFunc;
}

void InputLayout::AddVertexLayoutElement(
	std::string_view SemanticName,
	UINT			 SemanticIndex,
	UINT			 Location,
	ERHIFormat		 Format,
	UINT			 Stride)
{
	auto& Element		  = Elements.emplace_back();
	Element.SemanticName  = SemanticName.data();
	Element.SemanticIndex = SemanticIndex;
	Element.Location	  = Location;
	Element.Format		  = Format;
	Element.Stride		  = Stride;
}

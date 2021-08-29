#include "RHICommon.h"

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

void BlendState::AddRenderTargetForBlendOp(
	Factor	SrcBlendRGB,
	Factor	DstBlendRGB,
	BlendOp BlendOpRGB,
	Factor	SrcBlendAlpha,
	Factor	DstBlendAlpha,
	BlendOp BlendOpAlpha)
{
	if (m_NumRenderTargets >= 8)
		return;

	RenderTarget RenderTarget			  = {};
	RenderTarget.Operation				  = Operation::Blend;
	RenderTarget.SrcBlendRGB			  = SrcBlendRGB;
	RenderTarget.DstBlendRGB			  = DstBlendAlpha;
	RenderTarget.BlendOpRGB				  = BlendOpRGB;
	RenderTarget.SrcBlendAlpha			  = SrcBlendRGB;
	RenderTarget.DstBlendAlpha			  = DstBlendAlpha;
	RenderTarget.BlendOpAlpha			  = BlendOpRGB;
	m_RenderTargets[m_NumRenderTargets++] = RenderTarget;
}

void BlendState::AddRenderTargetForLogicOp(LogicOp LogicOpRGB)
{
	if (m_NumRenderTargets >= 8)
		return;

	RenderTarget RenderTarget			  = {};
	RenderTarget.Operation				  = Operation::Logic;
	RenderTarget.LogicOpRGB				  = LogicOpRGB;
	m_RenderTargets[m_NumRenderTargets++] = RenderTarget;
}

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
	Stencil& Stencil		   = Face == Face::Front ? m_FrontFace : m_BackFace;
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
	Stencil& Stencil	= Face == Face::Front ? m_FrontFace : m_BackFace;
	Stencil.StencilFunc = StencilFunc;
}

void InputLayout::AddVertexLayoutElement(
	std::string_view SemanticName,
	UINT			 SemanticIndex,
	UINT			 Location,
	ERHIFormat		 Format,
	UINT			 Stride)
{
	auto& Element		  = m_InputElements.emplace_back();
	Element.SemanticName  = SemanticName.data();
	Element.SemanticIndex = SemanticIndex;
	Element.Location	  = Location;
	Element.Format		  = Format;
	Element.Stride		  = Stride;
}

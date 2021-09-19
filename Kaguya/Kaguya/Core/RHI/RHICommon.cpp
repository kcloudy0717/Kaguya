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
	Factor	SrcBlendRgb,
	Factor	DstBlendRgb,
	BlendOp BlendOpRgb,
	Factor	SrcBlendAlpha,
	Factor	DstBlendAlpha,
	BlendOp BlendOpAlpha)
{
	RenderTarget RenderTarget  = {};
	RenderTarget.BlendEnable   = true;
	RenderTarget.SrcBlendRGB   = SrcBlendRgb;
	RenderTarget.DstBlendRGB   = DstBlendRgb;
	RenderTarget.BlendOpRGB	   = BlendOpRgb;
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

void RHIParsePipelineStream(const PipelineStateStreamDesc& Desc, IPipelineParserCallbacks* Callbacks)
{
	if (Desc.SizeInBytes == 0 || Desc.pPipelineStateSubobjectStream == nullptr)
	{
		Callbacks->ErrorBadInputParameter(1); // first parameter issue
		return;
	}

	bool SubobjectSeen[static_cast<UINT>(PipelineStateSubobjectType::NumTypes)] = {};
	for (SIZE_T CurOffset = 0, SizeOfSubobject = 0; CurOffset < Desc.SizeInBytes; CurOffset += SizeOfSubobject)
	{
		BYTE* Stream		= static_cast<BYTE*>(Desc.pPipelineStateSubobjectStream) + CurOffset;
		auto  SubobjectType = *reinterpret_cast<PipelineStateSubobjectType*>(Stream);
		UINT  Index			= static_cast<UINT>(SubobjectType);

		if (Index < 0 || Index >= static_cast<UINT>(PipelineStateSubobjectType::NumTypes))
		{
			Callbacks->ErrorUnknownSubobject(Index);
			return;
		}
		if (SubobjectSeen[Index])
		{
			Callbacks->ErrorDuplicateSubobject(SubobjectType);
			return; // disallow subobject duplicates in a stream
		}
		SubobjectSeen[Index] = true;

		switch (SubobjectType)
		{
		case PipelineStateSubobjectType::RootSignature:
			Callbacks->RootSignatureCb(*reinterpret_cast<PipelineStateStreamRootSignature*>(Stream));
			SizeOfSubobject = sizeof(PipelineStateStreamRootSignature);
			break;
		case PipelineStateSubobjectType::VS:
			Callbacks->VSCb(*reinterpret_cast<PipelineStateStreamVS*>(Stream));
			SizeOfSubobject = sizeof(PipelineStateStreamVS);
			break;
		case PipelineStateSubobjectType::PS:
			Callbacks->PSCb(*reinterpret_cast<PipelineStateStreamPS*>(Stream));
			SizeOfSubobject = sizeof(PipelineStateStreamPS);
			break;
		case PipelineStateSubobjectType::DS:
			Callbacks->DSCb(*reinterpret_cast<PipelineStateStreamDS*>(Stream));
			SizeOfSubobject = sizeof(PipelineStateStreamPS);
			break;
		case PipelineStateSubobjectType::HS:
			Callbacks->HSCb(*reinterpret_cast<PipelineStateStreamHS*>(Stream));
			SizeOfSubobject = sizeof(PipelineStateStreamHS);
			break;
		case PipelineStateSubobjectType::GS:
			Callbacks->GSCb(*reinterpret_cast<PipelineStateStreamGS*>(Stream));
			SizeOfSubobject = sizeof(PipelineStateStreamGS);
			break;
		case PipelineStateSubobjectType::CS:
			Callbacks->CSCb(*reinterpret_cast<PipelineStateStreamCS*>(Stream));
			SizeOfSubobject = sizeof(PipelineStateStreamCS);
			break;
		case PipelineStateSubobjectType::BlendState:
			Callbacks->BlendStateCb(*reinterpret_cast<PipelineStateStreamBlendState*>(Stream));
			SizeOfSubobject = sizeof(PipelineStateStreamBlendState);
			break;
		case PipelineStateSubobjectType::RasterizerState:
			Callbacks->RasterizerStateCb(*reinterpret_cast<PipelineStateStreamRasterizerState*>(Stream));
			SizeOfSubobject = sizeof(PipelineStateStreamRasterizerState);
			break;
		case PipelineStateSubobjectType::DepthStencilState:
			Callbacks->DepthStencilStateCb(*reinterpret_cast<PipelineStateStreamDepthStencilState*>(Stream));
			SizeOfSubobject = sizeof(PipelineStateStreamDepthStencilState);
			break;
		case PipelineStateSubobjectType::InputLayout:
			Callbacks->InputLayoutCb(*reinterpret_cast<PipelineStateStreamInputLayout*>(Stream));
			SizeOfSubobject = sizeof(PipelineStateStreamInputLayout);
			break;
		case PipelineStateSubobjectType::PrimitiveTopology:
			Callbacks->PrimitiveTopologyTypeCb(*reinterpret_cast<PipelineStateStreamPrimitiveTopology*>(Stream));
			SizeOfSubobject = sizeof(PipelineStateStreamPrimitiveTopology);
			break;
		case PipelineStateSubobjectType::RenderPass:
			Callbacks->RenderPassCb(*reinterpret_cast<PipelineStateStreamRenderPass*>(Stream));
			SizeOfSubobject = sizeof(PipelineStateStreamRenderPass);
			break;
		default:
			Callbacks->ErrorUnknownSubobject(Index);
			return;
		}
	}
}

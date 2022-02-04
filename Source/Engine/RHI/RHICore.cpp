#include "RHICore.h"

DEFINE_LOG_CATEGORY(RHI);

void RHIParsePipelineStream(const PipelineStateStreamDesc& Desc, IPipelineParserCallbacks* Callbacks)
{
	if (Desc.SizeInBytes == 0 || Desc.pPipelineStateSubobjectStream == nullptr)
	{
		Callbacks->ErrorBadInputParameter(1); // first parameter issue
		return;
	}

	bool SubobjectSeen[static_cast<size_t>(RHI_PIPELINE_STATE_SUBOBJECT_TYPE::NumTypes)] = {};
	for (size_t CurOffset = 0, SizeOfSubobject = 0; CurOffset < Desc.SizeInBytes; CurOffset += SizeOfSubobject)
	{
		std::byte* Stream		 = static_cast<std::byte*>(Desc.pPipelineStateSubobjectStream) + CurOffset;
		auto	   SubobjectType = *reinterpret_cast<RHI_PIPELINE_STATE_SUBOBJECT_TYPE*>(Stream);
		size_t	   Index		 = static_cast<size_t>(SubobjectType);

		if (Index < 0 || Index >= static_cast<size_t>(RHI_PIPELINE_STATE_SUBOBJECT_TYPE::NumTypes))
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
			using enum RHI_PIPELINE_STATE_SUBOBJECT_TYPE;
		case RootSignature:
			Callbacks->RootSignatureCb(*reinterpret_cast<PipelineStateStreamRootSignature*>(Stream));
			SizeOfSubobject = sizeof(PipelineStateStreamRootSignature);
			break;
		case InputLayout:
			Callbacks->InputLayoutCb(*reinterpret_cast<PipelineStateStreamInputLayout*>(Stream));
			SizeOfSubobject = sizeof(PipelineStateStreamInputLayout);
			break;
		case VS:
			Callbacks->VSCb(*reinterpret_cast<PipelineStateStreamVS*>(Stream));
			SizeOfSubobject = sizeof(PipelineStateStreamVS);
			break;
		case PS:
			Callbacks->PSCb(*reinterpret_cast<PipelineStateStreamPS*>(Stream));
			SizeOfSubobject = sizeof(PipelineStateStreamPS);
			break;
		case DS:
			Callbacks->DSCb(*reinterpret_cast<PipelineStateStreamDS*>(Stream));
			SizeOfSubobject = sizeof(PipelineStateStreamPS);
			break;
		case HS:
			Callbacks->HSCb(*reinterpret_cast<PipelineStateStreamHS*>(Stream));
			SizeOfSubobject = sizeof(PipelineStateStreamHS);
			break;
		case GS:
			Callbacks->GSCb(*reinterpret_cast<PipelineStateStreamGS*>(Stream));
			SizeOfSubobject = sizeof(PipelineStateStreamGS);
			break;
		case CS:
			Callbacks->CSCb(*reinterpret_cast<PipelineStateStreamCS*>(Stream));
			SizeOfSubobject = sizeof(PipelineStateStreamCS);
			break;
		case AS:
			Callbacks->ASCb(*reinterpret_cast<PipelineStateStreamMS*>(Stream));
			SizeOfSubobject = sizeof(PipelineStateStreamAS);
			break;
		case MS:
			Callbacks->MSCb(*reinterpret_cast<PipelineStateStreamMS*>(Stream));
			SizeOfSubobject = sizeof(PipelineStateStreamMS);
			break;
		case BlendState:
			Callbacks->BlendStateCb(*reinterpret_cast<PipelineStateStreamBlendState*>(Stream));
			SizeOfSubobject = sizeof(PipelineStateStreamBlendState);
			break;
		case RasterizerState:
			Callbacks->RasterizerStateCb(*reinterpret_cast<PipelineStateStreamRasterizerState*>(Stream));
			SizeOfSubobject = sizeof(PipelineStateStreamRasterizerState);
			break;
		case DepthStencilState:
			Callbacks->DepthStencilStateCb(*reinterpret_cast<PipelineStateStreamDepthStencilState*>(Stream));
			SizeOfSubobject = sizeof(PipelineStateStreamDepthStencilState);
			break;
		case RenderTargetState:
			Callbacks->RenderTargetStateCb(*reinterpret_cast<PipelineStateStreamRenderTargetState*>(Stream));
			SizeOfSubobject = sizeof(PipelineStateStreamRenderTargetState);
			break;
		case PrimitiveTopology:
			Callbacks->PrimitiveTopologyTypeCb(*reinterpret_cast<PipelineStateStreamPrimitiveTopology*>(Stream));
			SizeOfSubobject = sizeof(PipelineStateStreamPrimitiveTopology);
			break;
		default:
			Callbacks->ErrorUnknownSubobject(Index);
			return;
		}
	}
}

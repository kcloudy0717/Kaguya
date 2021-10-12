#include "D3D12PipelineState.h"

D3D12PipelineState::D3D12PipelineState(D3D12Device* Parent, const PipelineStateStreamDesc& Desc)
	: D3D12DeviceChild(Parent)
{
	D3D12PipelineParserCallbacks Parser;
	RHIParsePipelineStream(Desc, &Parser);

	if (Parser.Type == ED3D12PipelineStateType::Graphics)
	{
		Create<ED3D12PipelineStateType::Graphics>(Parser);
	}
	else if (Parser.Type == ED3D12PipelineStateType::Compute)
	{
		Create<ED3D12PipelineStateType::Compute>(Parser);
	}
}

ID3D12PipelineState* D3D12PipelineState::GetApiHandle() const noexcept
{
	if (CompilationWork)
	{
		CompilationWork->Wait(false);
		CompilationWork.reset();
	}
	return PipelineState.Get();
}

template<ED3D12PipelineStateType Type>
void D3D12PipelineState::Create(D3D12PipelineParserCallbacks Parser)
{
	D3D12Device* Parent = GetParentDevice();
	if (Parent->PsoCompilationThreadPool)
	{
		CompilationWork = std::make_unique<ThreadPoolWork>();
		Parent->PsoCompilationThreadPool->QueueThreadpoolWork(
			CompilationWork.get(),
			[=, this]()
			{
				try
				{
					InternalCreate<Type>(Parser);
				}
				catch (...)
				{
				}
			});
	}
	else
	{
		InternalCreate<Type>(Parser);
	}
}

template<ED3D12PipelineStateType Type>
void D3D12PipelineState::InternalCreate(const D3D12PipelineParserCallbacks& Parser)
{
	if constexpr (Type == ED3D12PipelineStateType::Graphics)
	{
		if (!Parser.MS)
		{
			D3D12_GRAPHICS_PIPELINE_STATE_DESC Desc = {};
			Desc.pRootSignature						= Parser.RootSignature->GetApiHandle();
			Desc.VS									= Parser.VS ? *Parser.VS : D3D12_SHADER_BYTECODE();
			Desc.PS									= Parser.PS ? *Parser.PS : D3D12_SHADER_BYTECODE();
			Desc.DS									= Parser.DS ? *Parser.DS : D3D12_SHADER_BYTECODE();
			Desc.HS									= Parser.HS ? *Parser.HS : D3D12_SHADER_BYTECODE();
			Desc.GS									= Parser.GS ? *Parser.GS : D3D12_SHADER_BYTECODE();
			Desc.StreamOutput						= D3D12_STREAM_OUTPUT_DESC();
			Desc.BlendState							= static_cast<D3D12_BLEND_DESC>(Parser.BlendState);
			Desc.SampleMask							= DefaultSampleMask();
			Desc.RasterizerState					= static_cast<D3D12_RASTERIZER_DESC>(Parser.RasterizerState);
			Desc.DepthStencilState					= static_cast<D3D12_DEPTH_STENCIL_DESC>(Parser.DepthStencilState);
			Desc.InputLayout						= static_cast<D3D12_INPUT_LAYOUT_DESC>(Parser.InputLayout);
			Desc.IBStripCutValue					= D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
			Desc.PrimitiveTopologyType				= ToD3D12PrimitiveTopologyType(Parser.PrimitiveTopology);
			Desc.NumRenderTargets					= Parser.RenderPass->Desc.NumRenderTargets;
			memcpy(Desc.RTVFormats, Parser.RenderPass->RenderTargetFormats.RTFormats, sizeof(Desc.RTVFormats));
			Desc.DSVFormat	= Parser.RenderPass->DepthStencilFormat;
			Desc.SampleDesc = DefaultSampleDesc();
			Desc.NodeMask	= 0;
			Desc.CachedPSO	= D3D12_CACHED_PIPELINE_STATE();
			Desc.Flags		= D3D12_PIPELINE_STATE_FLAG_NONE;

			VERIFY_D3D12_API(GetParentDevice()->GetD3D12Device5()->CreateGraphicsPipelineState(
				&Desc,
				IID_PPV_ARGS(PipelineState.ReleaseAndGetAddressOf())));
		}
		else
		{
			D3DX12_MESH_SHADER_PIPELINE_STATE_DESC Desc = {};
			Desc.pRootSignature							= Parser.RootSignature->GetApiHandle();
			Desc.MS										= Parser.MS ? *Parser.MS : D3D12_SHADER_BYTECODE();
			Desc.PS										= Parser.PS ? *Parser.PS : D3D12_SHADER_BYTECODE();
			Desc.BlendState								= static_cast<D3D12_BLEND_DESC>(Parser.BlendState);
			Desc.SampleMask								= DefaultSampleMask();
			Desc.RasterizerState						= static_cast<D3D12_RASTERIZER_DESC>(Parser.RasterizerState);
			Desc.DepthStencilState	   = static_cast<D3D12_DEPTH_STENCIL_DESC>(Parser.DepthStencilState);
			Desc.PrimitiveTopologyType = ToD3D12PrimitiveTopologyType(Parser.PrimitiveTopology);
			Desc.NumRenderTargets	   = Parser.RenderPass->Desc.NumRenderTargets;
			memcpy(Desc.RTVFormats, Parser.RenderPass->RenderTargetFormats.RTFormats, sizeof(Desc.RTVFormats));
			Desc.DSVFormat	= Parser.RenderPass->DepthStencilFormat;
			Desc.SampleDesc = DefaultSampleDesc();
			Desc.NodeMask	= 0;
			Desc.CachedPSO	= D3D12_CACHED_PIPELINE_STATE();
			Desc.Flags		= D3D12_PIPELINE_STATE_FLAG_NONE;

			auto							 Stream		= CD3DX12_PIPELINE_MESH_STATE_STREAM(Desc);
			D3D12_PIPELINE_STATE_STREAM_DESC StreamDesc = {};
			StreamDesc.pPipelineStateSubobjectStream	= &Stream;
			StreamDesc.SizeInBytes						= sizeof(Stream);

			VERIFY_D3D12_API(GetParentDevice()->GetD3D12Device5()->CreatePipelineState(
				&StreamDesc,
				IID_PPV_ARGS(PipelineState.ReleaseAndGetAddressOf())));
		}
	}
	else if constexpr (Type == ED3D12PipelineStateType::Compute)
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC Desc = {};
		Desc.pRootSignature					   = Parser.RootSignature->GetApiHandle();
		Desc.CS								   = *Parser.CS;
		Desc.NodeMask						   = 0;
		Desc.CachedPSO						   = D3D12_CACHED_PIPELINE_STATE();
		Desc.Flags							   = D3D12_PIPELINE_STATE_FLAG_NONE;

		VERIFY_D3D12_API(GetParentDevice()->GetD3D12Device5()->CreateComputePipelineState(
			&Desc,
			IID_PPV_ARGS(PipelineState.ReleaseAndGetAddressOf())));
	}
}

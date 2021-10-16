#include "D3D12PipelineState.h"

static VOID NTAPI CompileFromCoroutineHandle(
	_Inout_ PTP_CALLBACK_INSTANCE Instance,
	_Inout_opt_ PVOID			  Context,
	_Inout_ PTP_WORK			  Work) noexcept
{
	UNREFERENCED_PARAMETER(Instance);
	std::coroutine_handle<>::from_address(Context)();
	CloseThreadpoolWork(Work);
}

struct awaitable_CompilePsoThread
{
	[[nodiscard]] constexpr bool await_ready() const noexcept { return false; }

	void await_resume() const noexcept {}

	void await_suspend(std::coroutine_handle<> handle) const
	{
		Device->GetPsoCompilationThreadPool()->QueueThreadpoolWork(CompileFromCoroutineHandle, handle.address());
	}

	D3D12Device* Device;
};

D3D12PipelineState::D3D12PipelineState(D3D12Device* Parent, const PipelineStateStreamDesc& Desc)
	: D3D12DeviceChild(Parent)
{
	D3D12PipelineParserCallbacks Parser;
	RHIParsePipelineStream(Desc, &Parser);
	CompilationWork = Create(Parser.Type, Parser);
}

ID3D12PipelineState* D3D12PipelineState::GetApiHandle() const noexcept
{
	if (CompilationWork)
	{
		CompilationWork.Wait();
		CompilationWork = nullptr;
	}
	return PipelineState.Get();
}

AsyncAction D3D12PipelineState::Create(ED3D12PipelineStateType Type, D3D12PipelineParserCallbacks Parser)
{
	D3D12Device* Parent = GetParentDevice();
	if (Parent->AllowAsyncPsoCompilation())
	{
		co_await awaitable_CompilePsoThread{ Parent };
	}
	else
	{
		co_await std::suspend_never{};
	}

	if (Type == ED3D12PipelineStateType::Graphics)
	{
		if (!Parser.MS)
		{
			CompileGraphicsPipeline(Parser);
		}
		else
		{
			CompileMeshShaderPipeline(Parser);
		}
	}
	else if (Type == ED3D12PipelineStateType::Compute)
	{
		CompileComputePipline(Parser);
	}
}

void D3D12PipelineState::CompileGraphicsPipeline(const D3D12PipelineParserCallbacks& Parser)
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

void D3D12PipelineState::CompileMeshShaderPipeline(const D3D12PipelineParserCallbacks& Parser)
{
	// TODO: Amplification Shader

	D3DX12_MESH_SHADER_PIPELINE_STATE_DESC Desc = {};
	Desc.pRootSignature							= Parser.RootSignature->GetApiHandle();
	Desc.MS										= Parser.MS ? *Parser.MS : D3D12_SHADER_BYTECODE();
	Desc.PS										= Parser.PS ? *Parser.PS : D3D12_SHADER_BYTECODE();
	Desc.BlendState								= static_cast<D3D12_BLEND_DESC>(Parser.BlendState);
	Desc.SampleMask								= DefaultSampleMask();
	Desc.RasterizerState						= static_cast<D3D12_RASTERIZER_DESC>(Parser.RasterizerState);
	Desc.DepthStencilState						= static_cast<D3D12_DEPTH_STENCIL_DESC>(Parser.DepthStencilState);
	Desc.PrimitiveTopologyType					= ToD3D12PrimitiveTopologyType(Parser.PrimitiveTopology);
	Desc.NumRenderTargets						= Parser.RenderPass->Desc.NumRenderTargets;
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

void D3D12PipelineState::CompileComputePipline(const D3D12PipelineParserCallbacks& Parser)
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

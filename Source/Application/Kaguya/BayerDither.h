#pragma once
#include "RendererRegistry.h"
#include "RHI/RHI.h"
#include "RHI/HlslResourceHandle.h"
#include "RHI/RenderGraph/RenderGraph.h"
#include "View.h"

struct BayerDitherInputParameters
{
	RHI::RgResourceHandle Input;
	RHI::RgResourceHandle Srv;
};

struct BayerDitherParameters
{
	RHI::RgResourceHandle Output;
	RHI::RgResourceHandle Srv;
	RHI::RgResourceHandle Uav;
};

static BayerDitherParameters AddBayerDitherPass(RHI::RenderGraph& Graph, const View& View, BayerDitherInputParameters Inputs)
{
	assert(Inputs.Input.IsValid());
	assert(Inputs.Srv.IsValid());

	BayerDitherParameters BayerDitherArgs;

	constexpr DXGI_FORMAT Format = RHI::D3D12SwapChain::Format;

	BayerDitherArgs.Output = Graph.Create<RHI::D3D12Texture>(RHI::RgTextureDesc("Bayer Dither Output").SetFormat(Format).SetExtent(View.Width, View.Height, 1).SetAllowUnorderedAccess());
	BayerDitherArgs.Srv	   = Graph.Create<RHI::D3D12ShaderResourceView>(RHI::RgViewDesc().SetResource(BayerDitherArgs.Output).AsTextureSrv());
	BayerDitherArgs.Uav	   = Graph.Create<RHI::D3D12UnorderedAccessView>(RHI::RgViewDesc().SetResource(BayerDitherArgs.Output).AsTextureUav());

	Graph.AddRenderPass("Bayer Dither")
		.Read({ Inputs.Input })
		.Write({ &BayerDitherArgs.Output })
		.Execute(
			[=](RHI::RenderGraphRegistry& Registry, RHI::D3D12CommandContext& Context)
			{
				struct Parameters
				{
					HlslTexture2D	Input;
					HlslRWTexture2D Output;
				} Args;
				Args.Input	= Registry.Get<RHI::D3D12ShaderResourceView>(Inputs.Srv);
				Args.Output = Registry.Get<RHI::D3D12UnorderedAccessView>(BayerDitherArgs.Uav);

				Context.SetPipelineState(Registry.GetPipelineState(PipelineStates::BayerDither));
				Context.SetComputeRootSignature(Registry.GetRootSignature(RootSignatures::BayerDither));
				Context->SetComputeRoot32BitConstants(0, 2, &Args, 0);
				Context.Dispatch2D<8, 8>(View.Width, View.Height);
			});

	return BayerDitherArgs;
}

#pragma once
#include "RendererRegistry.h"
#include "RHI/RHI.h"
#include "RHI/RenderGraph/RenderGraph.h"
#include "View.h"

struct BayerDitherInputParameters
{
	RgResourceHandle Input;
	RgResourceHandle Srv;
};

struct BayerDitherParameters
{
	RgResourceHandle Output;
	RgResourceHandle Srv;
	RgResourceHandle Uav;
};

static BayerDitherParameters AddBayerDitherPass(RenderGraph& Graph, const View& View, BayerDitherInputParameters Inputs)
{
	assert(Inputs.Input.IsValid());
	assert(Inputs.Srv.IsValid());

	BayerDitherParameters BayerDitherArgs;

	constexpr DXGI_FORMAT Format = D3D12SwapChain::Format;

	BayerDitherArgs.Output = Graph.Create<D3D12Texture>("Bayer Dither Output", RgTextureDesc().SetFormat(Format).SetExtent(View.Width, View.Height, 1).AllowUnorderedAccess());
	BayerDitherArgs.Srv	   = Graph.Create<D3D12ShaderResourceView>("Bayer Dither Srv", RgViewDesc().SetResource(BayerDitherArgs.Output).AsTextureSrv());
	BayerDitherArgs.Uav	   = Graph.Create<D3D12UnorderedAccessView>("Bayer Dither Uav", RgViewDesc().SetResource(BayerDitherArgs.Output).AsTextureUav());

	Graph.AddRenderPass("Bayer Dither")
		.Read(Inputs.Input)
		.Write(&BayerDitherArgs.Output)
		.Execute([=](RenderGraphRegistry& Registry, D3D12CommandContext& Context)
				 {
					 struct Parameters
					 {
						 unsigned int InputIndex;
						 unsigned int OutputIndex;
					 } Args;
					 Args.InputIndex  = Registry.Get<D3D12ShaderResourceView>(Inputs.Srv)->GetIndex();
					 Args.OutputIndex = Registry.Get<D3D12UnorderedAccessView>(BayerDitherArgs.Uav)->GetIndex();

					 D3D12ScopedEvent(Context, "Bayer Dither");
					 Context.SetPipelineState(Registry.GetPipelineState(PipelineStates::BayerDither));
					 Context.SetComputeRootSignature(Registry.GetRootSignature(RootSignatures::BayerDither));
					 Context->SetComputeRoot32BitConstants(0, 2, &Args, 0);
					 Context.Dispatch2D<8, 8>(View.Width, View.Height);
				 });

	return BayerDitherArgs;
}

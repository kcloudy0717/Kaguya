#pragma once
#include "RendererRegistry.h"
#include "RHI/RHI.h"
#include "RHI/RenderGraph/RenderGraph.h"
#include "View.h"

struct SobolInputParameters
{
	RgResourceHandle Input;
	RgResourceHandle Srv;
};

struct SobolParameters
{
	RgResourceHandle Output;
	RgResourceHandle Srv;
	RgResourceHandle Uav;
};

static SobolParameters AddSobolPass(RenderGraph& Graph, const View& View, SobolInputParameters Inputs)
{
	assert(Inputs.Input.IsValid());
	assert(Inputs.Srv.IsValid());

	SobolParameters SobolArgs;

	constexpr DXGI_FORMAT Format = D3D12SwapChain::Format;

	SobolArgs.Output = Graph.Create<D3D12Texture>("Sobol Output", RgTextureDesc().SetFormat(Format).SetExtent(View.Width, View.Height, 1).AllowUnorderedAccess());
	SobolArgs.Srv	 = Graph.Create<D3D12ShaderResourceView>("Sobol Srv", RgViewDesc().SetResource(SobolArgs.Output).AsTextureSrv());
	SobolArgs.Uav	 = Graph.Create<D3D12UnorderedAccessView>("Sobol Uav", RgViewDesc().SetResource(SobolArgs.Output).AsTextureUav());

	Graph.AddRenderPass("Sobol")
		.Read(Inputs.Input)
		.Write(&SobolArgs.Output)
		.Execute([=](RenderGraphRegistry& Registry, D3D12CommandContext& Context)
				 {
					 struct Parameters
					 {
						 unsigned int InputIndex;
						 unsigned int OutputIndex;
					 } Args;
					 Args.InputIndex  = Registry.Get<D3D12ShaderResourceView>(Inputs.Srv)->GetIndex();
					 Args.OutputIndex = Registry.Get<D3D12UnorderedAccessView>(SobolArgs.Uav)->GetIndex();

					 D3D12ScopedEvent(Context, "Sobol");
					 Context.SetPipelineState(Registry.GetPipelineState(PipelineStates::Sobol));
					 Context.SetComputeRootSignature(Registry.GetRootSignature(RootSignatures::Sobol));
					 Context->SetComputeRoot32BitConstants(0, 2, &Args, 0);
					 Context.Dispatch2D<8, 8>(View.Width, View.Height);
				 });

	return SobolArgs;
}

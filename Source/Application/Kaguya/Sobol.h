#pragma once
#include "RendererRegistry.h"
#include "RHI/RHI.h"
#include "RHI/HlslResourceHandle.h"
#include "RHI/RenderGraph/RenderGraph.h"
#include "View.h"

struct SobolInputParameters
{
	RHI::RgResourceHandle Input;
	RHI::RgResourceHandle Srv;
};

struct SobolParameters
{
	RHI::RgResourceHandle Output;
	RHI::RgResourceHandle Srv;
	RHI::RgResourceHandle Uav;
};

static SobolParameters AddSobolPass(RHI::RenderGraph& Graph, const View& View, SobolInputParameters Inputs)
{
	assert(Inputs.Input.IsValid());
	assert(Inputs.Srv.IsValid());

	SobolParameters SobolArgs;

	constexpr DXGI_FORMAT Format = RHI::D3D12SwapChain::Format;

	SobolArgs.Output = Graph.Create<RHI::D3D12Texture>("Sobol Output", RHI::RgTextureDesc().SetFormat(Format).SetExtent(View.Width, View.Height, 1).AllowUnorderedAccess());
	SobolArgs.Srv	 = Graph.Create<RHI::D3D12ShaderResourceView>("Sobol Srv", RHI::RgViewDesc().SetResource(SobolArgs.Output).AsTextureSrv());
	SobolArgs.Uav	 = Graph.Create<RHI::D3D12UnorderedAccessView>("Sobol Uav", RHI::RgViewDesc().SetResource(SobolArgs.Output).AsTextureUav());

	Graph.AddRenderPass("Sobol")
		.Read(Inputs.Input)
		.Write(&SobolArgs.Output)
		.Execute(
			[=](RHI::RenderGraphRegistry& Registry, RHI::D3D12CommandContext& Context)
			{
				struct Parameters
				{
					HlslTexture2D	Input;
					HlslRWTexture2D Output;
				} Args;
				Args.Input	= Registry.Get<RHI::D3D12ShaderResourceView>(Inputs.Srv);
				Args.Output = Registry.Get<RHI::D3D12UnorderedAccessView>(SobolArgs.Uav);

				Context.SetPipelineState(Registry.GetPipelineState(PipelineStates::Sobol));
				Context.SetComputeRootSignature(Registry.GetRootSignature(RootSignatures::Sobol));
				Context->SetComputeRoot32BitConstants(0, 2, &Args, 0);
				Context.Dispatch2D<8, 8>(View.Width, View.Height);
			});

	return SobolArgs;
}

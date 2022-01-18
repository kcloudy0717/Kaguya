#pragma once
#include "RendererRegistry.h"
#include "RHI/RHI.h"
#include "RHI/RenderGraph/RenderGraph.h"
#include "View.h"
#include "Math/Math.h"

enum class TonemapOperator
{
	Aces
};

struct TonemapSettings
{
	TonemapOperator Operator	   = TonemapOperator::Aces;
	float			BloomIntensity = 2.0f;
};

struct TonemapInputParameters
{
	RgResourceHandle Input;
	RgResourceHandle Srv;
	RgResourceHandle BloomInput;
	RgResourceHandle BloomInputSrv;
};

struct TonemapParameters
{
	RgResourceHandle Output;
	RgResourceHandle Srv;
	RgResourceHandle Uav;
};

static TonemapParameters AddTonemapPass(RenderGraph& Graph, const View& View, TonemapInputParameters Inputs, TonemapSettings Settings = TonemapSettings())
{
	assert(Inputs.Input.IsValid());
	assert(Inputs.BloomInput.IsValid());
	assert(Inputs.Srv.IsValid());
	assert(Inputs.BloomInputSrv.IsValid());

	TonemapParameters TonemapArgs;

	constexpr DXGI_FORMAT Format = D3D12SwapChain::Format;

	TonemapArgs.Output = Graph.Create<D3D12Texture>("Tonemap Output", RgTextureDesc().SetFormat(Format).SetExtent(View.Width, View.Height, 1).AllowUnorderedAccess());
	TonemapArgs.Srv	   = Graph.Create<D3D12ShaderResourceView>("Tonemap Srv", RgViewDesc().SetResource(TonemapArgs.Output).AsTextureSrv());
	TonemapArgs.Uav	   = Graph.Create<D3D12UnorderedAccessView>("Tonemap Uav", RgViewDesc().SetResource(TonemapArgs.Output).AsTextureUav());

	Graph.AddRenderPass("Tonemap")
		.Read(Inputs.Input)
		.Read(Inputs.BloomInput)
		.Write(&TonemapArgs.Output)
		.Execute([=](RenderGraphRegistry& Registry, D3D12CommandContext& Context)
				 {
					 struct Parameters
					 {
						 Vec2f		  InverseOutputSize;
						 float		  BloomIntensity;
						 unsigned int InputIndex;
						 unsigned int BloomIndex;
						 unsigned int OutputIndex;
					 } Args;
					 Args.InverseOutputSize = Vec2f(1.0f / static_cast<float>(View.Width), 1.0f / static_cast<float>(View.Height));
					 Args.BloomIntensity	= Settings.BloomIntensity;
					 Args.InputIndex		= Registry.Get<D3D12ShaderResourceView>(Inputs.Srv)->GetIndex();
					 Args.BloomIndex		= Registry.Get<D3D12ShaderResourceView>(Inputs.BloomInputSrv)->GetIndex();
					 Args.OutputIndex		= Registry.Get<D3D12UnorderedAccessView>(TonemapArgs.Uav)->GetIndex();

					 Context.SetPipelineState(Registry.GetPipelineState(PipelineStates::Tonemap));
					 Context.SetComputeRootSignature(Registry.GetRootSignature(RootSignatures::Tonemap));
					 Context->SetComputeRoot32BitConstants(0, 6, &Args, 0);
					 Context.Dispatch2D<8, 8>(View.Width, View.Height);
				 });

	return TonemapArgs;
}

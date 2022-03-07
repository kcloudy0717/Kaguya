#pragma once
#include "RendererRegistry.h"
#include "RHI/RHI.h"
#include "RHI/HlslResourceHandle.h"
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
	RHI::RgResourceHandle Input;
	RHI::RgResourceHandle Srv;
	RHI::RgResourceHandle BloomInput;
	RHI::RgResourceHandle BloomInputSrv;
};

struct TonemapParameters
{
	RHI::RgResourceHandle Output;
	RHI::RgResourceHandle Srv;
	RHI::RgResourceHandle Uav;
};

static TonemapParameters AddTonemapPass(RHI::RenderGraph& Graph, const View& View, TonemapInputParameters Inputs, TonemapSettings Settings = TonemapSettings())
{
	assert(Inputs.Input.IsValid());
	assert(Inputs.BloomInput.IsValid());
	assert(Inputs.Srv.IsValid());
	assert(Inputs.BloomInputSrv.IsValid());

	TonemapParameters TonemapArgs;

	constexpr DXGI_FORMAT Format = RHI::D3D12SwapChain::Format;

	TonemapArgs.Output = Graph.Create<RHI::D3D12Texture>(RHI::RgTextureDesc("Tonemap Output").SetFormat(Format).SetExtent(View.Width, View.Height, 1).AllowUnorderedAccess());
	TonemapArgs.Srv	   = Graph.Create<RHI::D3D12ShaderResourceView>(RHI::RgViewDesc().SetResource(TonemapArgs.Output).AsTextureSrv());
	TonemapArgs.Uav	   = Graph.Create<RHI::D3D12UnorderedAccessView>(RHI::RgViewDesc().SetResource(TonemapArgs.Output).AsTextureUav());

	Graph.AddRenderPass("Tonemap")
		.Read(Inputs.Input)
		.Read(Inputs.BloomInput)
		.Write(&TonemapArgs.Output)
		.Execute(
			[=](RHI::RenderGraphRegistry& Registry, RHI::D3D12CommandContext& Context)
			{
				struct Parameters
				{
					Vec2f			InverseOutputSize;
					float			BloomIntensity;
					HlslTexture2D	Input;
					HlslTexture2D	Bloom;
					HlslRWTexture2D Output;
				} Args;
				Args.InverseOutputSize = Vec2f(1.0f / static_cast<float>(View.Width), 1.0f / static_cast<float>(View.Height));
				Args.BloomIntensity	   = Settings.BloomIntensity;
				Args.Input			   = Registry.Get<RHI::D3D12ShaderResourceView>(Inputs.Srv);
				Args.Bloom			   = Registry.Get<RHI::D3D12ShaderResourceView>(Inputs.BloomInputSrv);
				Args.Output			   = Registry.Get<RHI::D3D12UnorderedAccessView>(TonemapArgs.Uav);

				Context.SetPipelineState(Registry.GetPipelineState(PipelineStates::Tonemap));
				Context.SetComputeRootSignature(Registry.GetRootSignature(RootSignatures::Tonemap));
				Context->SetComputeRoot32BitConstants(0, 6, &Args, 0);
				Context.Dispatch2D<8, 8>(View.Width, View.Height);
			});

	return TonemapArgs;
}

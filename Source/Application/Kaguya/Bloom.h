#pragma once
#include "RendererRegistry.h"
#include "RHI/RHI.h"
#include "RHI/HlslResourceHandle.h"
#include "RHI/RenderGraph/RenderGraph.h"
#include "View.h"
#include "Math/Math.h"

struct BloomSettings
{
	float Threshold = 3.5f;
};

struct BloomInputParameters
{
	RHI::RgResourceHandle Input;
	RHI::RgResourceHandle Srv;
};

struct BloomParameters
{
	// 0: Bloom downsampled, 1: Bloom blurred and upscaled
	RHI::RgResourceHandle Output1[2]; // 640x384 (1/3)
	RHI::RgResourceHandle Output2[2]; // 320x192 (1/6)
	RHI::RgResourceHandle Output3[2]; // 160x96  (1/12)
	RHI::RgResourceHandle Output4[2]; // 80x48   (1/24)
	RHI::RgResourceHandle Output5[2]; // 40x24   (1/48)

	RHI::RgResourceHandle Output1Srvs[2];
	RHI::RgResourceHandle Output2Srvs[2];
	RHI::RgResourceHandle Output3Srvs[2];
	RHI::RgResourceHandle Output4Srvs[2];
	RHI::RgResourceHandle Output5Srvs[2];
	RHI::RgResourceHandle Output1Uavs[2];
	RHI::RgResourceHandle Output2Uavs[2];
	RHI::RgResourceHandle Output3Uavs[2];
	RHI::RgResourceHandle Output4Uavs[2];
	RHI::RgResourceHandle Output5Uavs[2];
};

struct BlurUpsampleInputParameters
{
	RHI::RgResourceHandle  HighResInput; // [0]
	RHI::RgResourceHandle  LowResInput;
	RHI::RgResourceHandle* HighResOutput; // [1]

	RHI::RgResourceHandle HighResInputSrv;
	RHI::RgResourceHandle LowResInputSrv;
	RHI::RgResourceHandle HighResOutputUav;
};

static void BlurUpsample(std::string_view Name, RHI::RenderGraph& Graph, BlurUpsampleInputParameters Inputs, float UpsampleInterpolationFactor)
{
	Graph.AddRenderPass(Name)
		.Read(Inputs.HighResInput)
		.Read(Inputs.LowResInput)
		.Write(Inputs.HighResOutput)
		.Execute(
			[=](RHI::RenderGraphRegistry& Registry, RHI::D3D12CommandContext& Context)
			{
				RHI::D3D12Texture* HighResInput = Registry.Get<RHI::D3D12Texture>(Inputs.HighResInput);
				struct Parameters
				{
					Vec2f			InverseOutputSize;
					float			UpsampleInterpolationFactor;
					HlslTexture2D	HighRes;
					HlslTexture2D	LowRes;
					HlslRWTexture2D Output;
				} Args;
				Args.InverseOutputSize			 = Vec2f(1.0f / static_cast<float>(HighResInput->GetDesc().Width), 1.0f / static_cast<float>(HighResInput->GetDesc().Height));
				Args.UpsampleInterpolationFactor = UpsampleInterpolationFactor;
				Args.HighRes					 = Registry.Get<RHI::D3D12ShaderResourceView>(Inputs.HighResInputSrv);
				Args.LowRes						 = Registry.Get<RHI::D3D12ShaderResourceView>(Inputs.LowResInputSrv);
				Args.Output						 = Registry.Get<RHI::D3D12UnorderedAccessView>(Inputs.HighResOutputUav);

				Context.SetPipelineState(Registry.GetPipelineState(PipelineStates::BloomUpsampleBlur));
				Context.SetComputeRootSignature(Registry.GetRootSignature(RootSignatures::BloomUpsampleBlur));
				Context->SetComputeRoot32BitConstants(0, 6, &Args, 0);
				Context.Dispatch2D<8, 8>(static_cast<UINT>(HighResInput->GetDesc().Width), HighResInput->GetDesc().Height);
			});
}

static BloomParameters AddBloomPass(RHI::RenderGraph& Graph, const View& View, BloomInputParameters Inputs, BloomSettings Settings = BloomSettings())
{
	uint32_t kBloomWidth  = View.Width > 2560 ? 1280 : 640;
	uint32_t kBloomHeight = View.Height > 1440 ? 768 : 384;

	BloomParameters BloomArgs;

	constexpr DXGI_FORMAT Format = DXGI_FORMAT_R11G11B10_FLOAT;
	BloomArgs.Output1[0]		 = Graph.Create<RHI::D3D12Texture>(RHI::RgTextureDesc("Bloom Output 1a").SetFormat(Format).SetExtent(kBloomWidth, kBloomHeight, 1).SetAllowUnorderedAccess());
	BloomArgs.Output1[1]		 = Graph.Create<RHI::D3D12Texture>(RHI::RgTextureDesc("Bloom Output 1b").SetFormat(Format).SetExtent(kBloomWidth, kBloomHeight, 1).SetAllowUnorderedAccess());
	BloomArgs.Output2[0]		 = Graph.Create<RHI::D3D12Texture>(RHI::RgTextureDesc("Bloom Output 2a").SetFormat(Format).SetExtent(kBloomWidth / 2, kBloomHeight / 2, 1).SetAllowUnorderedAccess());
	BloomArgs.Output2[1]		 = Graph.Create<RHI::D3D12Texture>(RHI::RgTextureDesc("Bloom Output 2b").SetFormat(Format).SetExtent(kBloomWidth / 2, kBloomHeight / 2, 1).SetAllowUnorderedAccess());
	BloomArgs.Output3[0]		 = Graph.Create<RHI::D3D12Texture>(RHI::RgTextureDesc("Bloom Output 3a").SetFormat(Format).SetExtent(kBloomWidth / 4, kBloomHeight / 4, 1).SetAllowUnorderedAccess());
	BloomArgs.Output3[1]		 = Graph.Create<RHI::D3D12Texture>(RHI::RgTextureDesc("Bloom Output 3b").SetFormat(Format).SetExtent(kBloomWidth / 4, kBloomHeight / 4, 1).SetAllowUnorderedAccess());
	BloomArgs.Output4[0]		 = Graph.Create<RHI::D3D12Texture>(RHI::RgTextureDesc("Bloom Output 4a").SetFormat(Format).SetExtent(kBloomWidth / 8, kBloomHeight / 8, 1).SetAllowUnorderedAccess());
	BloomArgs.Output4[1]		 = Graph.Create<RHI::D3D12Texture>(RHI::RgTextureDesc("Bloom Output 4b").SetFormat(Format).SetExtent(kBloomWidth / 8, kBloomHeight / 8, 1).SetAllowUnorderedAccess());
	BloomArgs.Output5[0]		 = Graph.Create<RHI::D3D12Texture>(RHI::RgTextureDesc("Bloom Output 5a").SetFormat(Format).SetExtent(kBloomWidth / 16, kBloomHeight / 16, 1).SetAllowUnorderedAccess());
	BloomArgs.Output5[1]		 = Graph.Create<RHI::D3D12Texture>(RHI::RgTextureDesc("Bloom Output 5b").SetFormat(Format).SetExtent(kBloomWidth / 16, kBloomHeight / 16, 1).SetAllowUnorderedAccess());

	for (size_t i = 0; i < 2; ++i)
	{
		// Srvs
		BloomArgs.Output1Srvs[i] = Graph.Create<RHI::D3D12ShaderResourceView>(RHI::RgViewDesc().SetResource(BloomArgs.Output1[i]).AsTextureSrv());
		BloomArgs.Output2Srvs[i] = Graph.Create<RHI::D3D12ShaderResourceView>(RHI::RgViewDesc().SetResource(BloomArgs.Output2[i]).AsTextureSrv());
		BloomArgs.Output3Srvs[i] = Graph.Create<RHI::D3D12ShaderResourceView>(RHI::RgViewDesc().SetResource(BloomArgs.Output3[i]).AsTextureSrv());
		BloomArgs.Output4Srvs[i] = Graph.Create<RHI::D3D12ShaderResourceView>(RHI::RgViewDesc().SetResource(BloomArgs.Output4[i]).AsTextureSrv());
		BloomArgs.Output5Srvs[i] = Graph.Create<RHI::D3D12ShaderResourceView>(RHI::RgViewDesc().SetResource(BloomArgs.Output5[i]).AsTextureSrv());
		// Uavs
		BloomArgs.Output1Uavs[i] = Graph.Create<RHI::D3D12UnorderedAccessView>(RHI::RgViewDesc().SetResource(BloomArgs.Output1[i]).AsTextureUav());
		BloomArgs.Output2Uavs[i] = Graph.Create<RHI::D3D12UnorderedAccessView>(RHI::RgViewDesc().SetResource(BloomArgs.Output2[i]).AsTextureUav());
		BloomArgs.Output3Uavs[i] = Graph.Create<RHI::D3D12UnorderedAccessView>(RHI::RgViewDesc().SetResource(BloomArgs.Output3[i]).AsTextureUav());
		BloomArgs.Output4Uavs[i] = Graph.Create<RHI::D3D12UnorderedAccessView>(RHI::RgViewDesc().SetResource(BloomArgs.Output4[i]).AsTextureUav());
		BloomArgs.Output5Uavs[i] = Graph.Create<RHI::D3D12UnorderedAccessView>(RHI::RgViewDesc().SetResource(BloomArgs.Output5[i]).AsTextureUav());
	}

	Graph.AddRenderPass("Bloom Mask")
		.Read(Inputs.Input)
		.Write(&BloomArgs.Output1[0])
		.Execute(
			[=](RHI::RenderGraphRegistry& Registry, RHI::D3D12CommandContext& Context)
			{
				struct Parameters
				{
					Vec2f			InverseOutputSize;
					float			Threshold;
					HlslTexture2D	Input;
					HlslRWTexture2D Output;
				} Args;
				Args.InverseOutputSize = Vec2f(1.0f / static_cast<float>(kBloomWidth), 1.0f / static_cast<float>(kBloomHeight));
				Args.Threshold		   = Settings.Threshold;
				Args.Input			   = Registry.Get<RHI::D3D12ShaderResourceView>(Inputs.Srv);
				Args.Output			   = Registry.Get<RHI::D3D12UnorderedAccessView>(BloomArgs.Output1Uavs[0]);

				Context.SetPipelineState(Registry.GetPipelineState(PipelineStates::BloomMask));
				Context.SetComputeRootSignature(Registry.GetRootSignature(RootSignatures::BloomMask));
				Context->SetComputeRoot32BitConstants(0, 5, &Args, 0);
				Context.Dispatch2D<8, 8>(kBloomWidth, kBloomHeight);
			});
	Graph.AddRenderPass("Bloom Downsample")
		.Read(BloomArgs.Output1[0])
		.Write(&BloomArgs.Output2[0])
		.Write(&BloomArgs.Output3[0])
		.Write(&BloomArgs.Output4[0])
		.Write(&BloomArgs.Output5[0])
		.Execute(
			[=](RHI::RenderGraphRegistry& Registry, RHI::D3D12CommandContext& Context)
			{
				struct Parameters
				{
					Vec2f			InverseOutputSize;
					HlslTexture2D	Bloom;
					HlslRWTexture2D Output1;
					HlslRWTexture2D Output2;
					HlslRWTexture2D Output3;
					HlslRWTexture2D Output4;
				} Args;
				Args.InverseOutputSize = Vec2f(1.0f / static_cast<float>(kBloomWidth), 1.0f / static_cast<float>(kBloomHeight));
				Args.Bloom			   = Registry.Get<RHI::D3D12ShaderResourceView>(BloomArgs.Output1Srvs[0]);
				Args.Output1		   = Registry.Get<RHI::D3D12UnorderedAccessView>(BloomArgs.Output2Uavs[0]);
				Args.Output2		   = Registry.Get<RHI::D3D12UnorderedAccessView>(BloomArgs.Output3Uavs[0]);
				Args.Output3		   = Registry.Get<RHI::D3D12UnorderedAccessView>(BloomArgs.Output4Uavs[0]);
				Args.Output4		   = Registry.Get<RHI::D3D12UnorderedAccessView>(BloomArgs.Output5Uavs[0]);

				Context.SetPipelineState(Registry.GetPipelineState(PipelineStates::BloomDownsample));
				Context.SetComputeRootSignature(Registry.GetRootSignature(RootSignatures::BloomDownsample));
				Context->SetComputeRoot32BitConstants(0, 7, &Args, 0);
				Context.Dispatch2D<8, 8>(kBloomWidth / 2, kBloomHeight / 2);
			});
	Graph.AddRenderPass("Bloom Blur")
		.Read(BloomArgs.Output5[0])
		.Write(&BloomArgs.Output5[1])
		.Execute(
			[=](RHI::RenderGraphRegistry& Registry, RHI::D3D12CommandContext& Context)
			{
				RHI::D3D12Texture* Output5a = Registry.Get<RHI::D3D12Texture>(BloomArgs.Output5[0]);
				struct Parameters
				{
					HlslTexture2D	Input;
					HlslRWTexture2D Output;
				} Args;
				Args.Input	= Registry.Get<RHI::D3D12ShaderResourceView>(BloomArgs.Output5Srvs[0]);
				Args.Output = Registry.Get<RHI::D3D12UnorderedAccessView>(BloomArgs.Output5Uavs[1]);

				Context.SetPipelineState(Registry.GetPipelineState(PipelineStates::BloomBlur));
				Context.SetComputeRootSignature(Registry.GetRootSignature(RootSignatures::BloomBlur));
				Context->SetComputeRoot32BitConstants(0, 2, &Args, 0);
				Context.Dispatch2D<8, 8>(static_cast<UINT>(Output5a->GetDesc().Width), Output5a->GetDesc().Height);
			});

	BlurUpsampleInputParameters UpsampleArgs;

	// 4
	UpsampleArgs.HighResInput	  = BloomArgs.Output4[0];
	UpsampleArgs.HighResOutput	  = &BloomArgs.Output4[1];
	UpsampleArgs.LowResInput	  = BloomArgs.Output5[1];
	UpsampleArgs.HighResInputSrv  = BloomArgs.Output4Srvs[0];
	UpsampleArgs.HighResOutputUav = BloomArgs.Output4Uavs[1];
	UpsampleArgs.LowResInputSrv	  = BloomArgs.Output5Srvs[1];
	BlurUpsample("Bloom Upsample 4", Graph, UpsampleArgs, 0.65f);
	// 3
	UpsampleArgs.HighResInput	  = BloomArgs.Output3[0];
	UpsampleArgs.HighResOutput	  = &BloomArgs.Output3[1];
	UpsampleArgs.LowResInput	  = BloomArgs.Output4[1];
	UpsampleArgs.HighResInputSrv  = BloomArgs.Output3Srvs[0];
	UpsampleArgs.HighResOutputUav = BloomArgs.Output3Uavs[1];
	UpsampleArgs.LowResInputSrv	  = BloomArgs.Output4Srvs[1];
	BlurUpsample("Bloom Upsample 3", Graph, UpsampleArgs, 0.65f);
	// 2
	UpsampleArgs.HighResInput	  = BloomArgs.Output2[0];
	UpsampleArgs.HighResOutput	  = &BloomArgs.Output2[1];
	UpsampleArgs.LowResInput	  = BloomArgs.Output3[1];
	UpsampleArgs.HighResInputSrv  = BloomArgs.Output2Srvs[0];
	UpsampleArgs.HighResOutputUav = BloomArgs.Output2Uavs[1];
	UpsampleArgs.LowResInputSrv	  = BloomArgs.Output3Srvs[1];
	BlurUpsample("Bloom Upsample 2", Graph, UpsampleArgs, 0.65f);
	// 1
	UpsampleArgs.HighResInput	  = BloomArgs.Output1[0];
	UpsampleArgs.HighResOutput	  = &BloomArgs.Output1[1];
	UpsampleArgs.LowResInput	  = BloomArgs.Output2[1];
	UpsampleArgs.HighResInputSrv  = BloomArgs.Output1Srvs[0];
	UpsampleArgs.HighResOutputUav = BloomArgs.Output1Uavs[1];
	UpsampleArgs.LowResInputSrv	  = BloomArgs.Output2Srvs[1];
	BlurUpsample("Bloom Upsample 1", Graph, UpsampleArgs, 0.65f);

	return BloomArgs;
}

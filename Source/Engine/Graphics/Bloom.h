#pragma once
#include "RendererRegistry.h"
#include "RenderGraph/RenderGraph.h"
#include "View.h"
#include "Math.h"

struct BloomInputParameters
{
	RgResourceHandle Input;
	RgResourceHandle Srv;
};

struct BloomParameters
{
	// 0: Bloom downsampled, 1: Bloom blurred and upscaled
	RgResourceHandle Output1[2]; // 640x384 (1/3)
	RgResourceHandle Output2[2]; // 320x192 (1/6)
	RgResourceHandle Output3[2]; // 160x96  (1/12)
	RgResourceHandle Output4[2]; // 80x48   (1/24)
	RgResourceHandle Output5[2]; // 40x24   (1/48)

	RgResourceHandle Output1Srvs[2];
	RgResourceHandle Output2Srvs[2];
	RgResourceHandle Output3Srvs[2];
	RgResourceHandle Output4Srvs[2];
	RgResourceHandle Output5Srvs[2];
	RgResourceHandle Output1Uavs[2];
	RgResourceHandle Output2Uavs[2];
	RgResourceHandle Output3Uavs[2];
	RgResourceHandle Output4Uavs[2];
	RgResourceHandle Output5Uavs[2];
};

struct BlurUpsampleInputParameters
{
	RgResourceHandle  HighResInput; // [0]
	RgResourceHandle  LowResInput;
	RgResourceHandle* HighResOutput; // [1]

	RgResourceHandle HighResInputSrv;
	RgResourceHandle LowResInputSrv;
	RgResourceHandle HighResOutputUav;
};

static void BlurUpsample(std::string_view Name, RenderGraph& Graph, BlurUpsampleInputParameters Inputs, float UpsampleInterpolationFactor)
{
	Graph.AddRenderPass(Name)
		.Read(Inputs.HighResInput)
		.Read(Inputs.LowResInput)
		.Write(Inputs.HighResOutput)
		.Execute([=](RenderGraphRegistry& Registry, D3D12CommandContext& Context)
				 {
					 D3D12Texture* HighResInput = Registry.Get<D3D12Texture>(Inputs.HighResInput);
					 struct Parameters
					 {
						 Vec2f		  InverseOutputSize;
						 float		  UpsampleInterpolationFactor;
						 unsigned int HighResolutionIndex;
						 unsigned int LowResolutionIndex;
						 unsigned int OutputIndex;
					 } Args;
					 Args.InverseOutputSize			  = Vec2f(1.0f / static_cast<float>(HighResInput->GetDesc().Width), 1.0f / static_cast<float>(HighResInput->GetDesc().Height));
					 Args.UpsampleInterpolationFactor = UpsampleInterpolationFactor;
					 Args.HighResolutionIndex		  = Registry.Get<D3D12ShaderResourceView>(Inputs.HighResInputSrv)->GetIndex();
					 Args.LowResolutionIndex		  = Registry.Get<D3D12ShaderResourceView>(Inputs.LowResInputSrv)->GetIndex();
					 Args.OutputIndex				  = Registry.Get<D3D12UnorderedAccessView>(Inputs.HighResOutputUav)->GetIndex();

					 D3D12ScopedEvent(Context, Name);
					 Context.SetPipelineState(Registry.GetPipelineState(PipelineStates::BloomUpsampleBlur));
					 Context.SetComputeRootSignature(Registry.GetRootSignature(RootSignatures::BloomUpsampleBlur));
					 Context->SetComputeRoot32BitConstants(0, 6, &Args, 0);
					 Context.Dispatch2D<8, 8>(HighResInput->GetDesc().Width, HighResInput->GetDesc().Height);
				 });
}

static BloomParameters AddBloomPass(RenderGraph& Graph, const View& View, BloomInputParameters Inputs)
{
	uint32_t kBloomWidth  = View.Width > 2560 ? 1280 : 640;
	uint32_t kBloomHeight = View.Height > 1440 ? 768 : 384;

	BloomParameters BloomArgs;

	constexpr DXGI_FORMAT Format = DXGI_FORMAT_R11G11B10_FLOAT;
	BloomArgs.Output1[0]		 = Graph.Create<D3D12Texture>("Bloom Output 1a", RgTextureDesc().SetFormat(Format).SetExtent(kBloomWidth, kBloomHeight, 1).AllowUnorderedAccess());
	BloomArgs.Output1[1]		 = Graph.Create<D3D12Texture>("Bloom Output 1b", RgTextureDesc().SetFormat(Format).SetExtent(kBloomWidth, kBloomHeight, 1).AllowUnorderedAccess());
	BloomArgs.Output2[0]		 = Graph.Create<D3D12Texture>("Bloom Output 2a", RgTextureDesc().SetFormat(Format).SetExtent(kBloomWidth / 2, kBloomHeight / 2, 1).AllowUnorderedAccess());
	BloomArgs.Output2[1]		 = Graph.Create<D3D12Texture>("Bloom Output 2b", RgTextureDesc().SetFormat(Format).SetExtent(kBloomWidth / 2, kBloomHeight / 2, 1).AllowUnorderedAccess());
	BloomArgs.Output3[0]		 = Graph.Create<D3D12Texture>("Bloom Output 3a", RgTextureDesc().SetFormat(Format).SetExtent(kBloomWidth / 4, kBloomHeight / 4, 1).AllowUnorderedAccess());
	BloomArgs.Output3[1]		 = Graph.Create<D3D12Texture>("Bloom Output 3b", RgTextureDesc().SetFormat(Format).SetExtent(kBloomWidth / 4, kBloomHeight / 4, 1).AllowUnorderedAccess());
	BloomArgs.Output4[0]		 = Graph.Create<D3D12Texture>("Bloom Output 4a", RgTextureDesc().SetFormat(Format).SetExtent(kBloomWidth / 8, kBloomHeight / 8, 1).AllowUnorderedAccess());
	BloomArgs.Output4[1]		 = Graph.Create<D3D12Texture>("Bloom Output 4b", RgTextureDesc().SetFormat(Format).SetExtent(kBloomWidth / 8, kBloomHeight / 8, 1).AllowUnorderedAccess());
	BloomArgs.Output5[0]		 = Graph.Create<D3D12Texture>("Bloom Output 5a", RgTextureDesc().SetFormat(Format).SetExtent(kBloomWidth / 16, kBloomHeight / 16, 1).AllowUnorderedAccess());
	BloomArgs.Output5[1]		 = Graph.Create<D3D12Texture>("Bloom Output 5b", RgTextureDesc().SetFormat(Format).SetExtent(kBloomWidth / 16, kBloomHeight / 16, 1).AllowUnorderedAccess());

	for (size_t i = 0; i < 2; ++i)
	{
		// Srvs
		BloomArgs.Output1Srvs[i] = Graph.Create<D3D12ShaderResourceView>("Bloom Srv", RgViewDesc().SetResource(BloomArgs.Output1[i]).AsTextureSrv());
		BloomArgs.Output2Srvs[i] = Graph.Create<D3D12ShaderResourceView>("Bloom Srv", RgViewDesc().SetResource(BloomArgs.Output2[i]).AsTextureSrv());
		BloomArgs.Output3Srvs[i] = Graph.Create<D3D12ShaderResourceView>("Bloom Srv", RgViewDesc().SetResource(BloomArgs.Output3[i]).AsTextureSrv());
		BloomArgs.Output4Srvs[i] = Graph.Create<D3D12ShaderResourceView>("Bloom Srv", RgViewDesc().SetResource(BloomArgs.Output4[i]).AsTextureSrv());
		BloomArgs.Output5Srvs[i] = Graph.Create<D3D12ShaderResourceView>("Bloom Srv", RgViewDesc().SetResource(BloomArgs.Output5[i]).AsTextureSrv());
		// Uavs
		BloomArgs.Output1Uavs[i] = Graph.Create<D3D12UnorderedAccessView>("Bloom Uav", RgViewDesc().SetResource(BloomArgs.Output1[i]).AsTextureUav());
		BloomArgs.Output2Uavs[i] = Graph.Create<D3D12UnorderedAccessView>("Bloom Uav", RgViewDesc().SetResource(BloomArgs.Output2[i]).AsTextureUav());
		BloomArgs.Output3Uavs[i] = Graph.Create<D3D12UnorderedAccessView>("Bloom Uav", RgViewDesc().SetResource(BloomArgs.Output3[i]).AsTextureUav());
		BloomArgs.Output4Uavs[i] = Graph.Create<D3D12UnorderedAccessView>("Bloom Uav", RgViewDesc().SetResource(BloomArgs.Output4[i]).AsTextureUav());
		BloomArgs.Output5Uavs[i] = Graph.Create<D3D12UnorderedAccessView>("Bloom Uav", RgViewDesc().SetResource(BloomArgs.Output5[i]).AsTextureUav());
	}

	Graph.AddRenderPass("Bloom Mask")
		.Read(Inputs.Input)
		.Write(&BloomArgs.Output1[0])
		.Execute([=](RenderGraphRegistry& Registry, D3D12CommandContext& Context)
				 {
					 struct Parameters
					 {
						 Vec2f		  InverseOutputSize;
						 float		  Threshold;
						 unsigned int InputIndex;
						 unsigned int OutputIndex;
					 } Args;
					 Args.InverseOutputSize = Vec2f(1.0f / static_cast<float>(kBloomWidth), 1.0f / static_cast<float>(kBloomHeight));
					 Args.Threshold			= 1.0f;
					 Args.InputIndex		= Registry.Get<D3D12ShaderResourceView>(Inputs.Srv)->GetIndex();
					 Args.OutputIndex		= Registry.Get<D3D12UnorderedAccessView>(BloomArgs.Output1Uavs[0])->GetIndex();

					 D3D12ScopedEvent(Context, "Bloom Mask");
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
		.Execute([=](RenderGraphRegistry& Registry, D3D12CommandContext& Context)
				 {
					 struct Parameters
					 {
						 Vec2f		  InverseOutputSize;
						 unsigned int BloomIndex;
						 unsigned int Output1Index;
						 unsigned int Output2Index;
						 unsigned int Output3Index;
						 unsigned int Output4Index;
					 } Args;
					 Args.InverseOutputSize = Vec2f(1.0f / static_cast<float>(kBloomWidth), 1.0f / static_cast<float>(kBloomHeight));
					 Args.BloomIndex		= Registry.Get<D3D12ShaderResourceView>(BloomArgs.Output1Srvs[0])->GetIndex();
					 Args.Output1Index		= Registry.Get<D3D12UnorderedAccessView>(BloomArgs.Output2Uavs[0])->GetIndex();
					 Args.Output2Index		= Registry.Get<D3D12UnorderedAccessView>(BloomArgs.Output3Uavs[0])->GetIndex();
					 Args.Output3Index		= Registry.Get<D3D12UnorderedAccessView>(BloomArgs.Output4Uavs[0])->GetIndex();
					 Args.Output4Index		= Registry.Get<D3D12UnorderedAccessView>(BloomArgs.Output5Uavs[0])->GetIndex();

					 D3D12ScopedEvent(Context, "Bloom Downsample");
					 Context.SetPipelineState(Registry.GetPipelineState(PipelineStates::BloomDownsample));
					 Context.SetComputeRootSignature(Registry.GetRootSignature(RootSignatures::BloomDownsample));
					 Context->SetComputeRoot32BitConstants(0, 7, &Args, 0);
					 Context.Dispatch2D<8, 8>(kBloomWidth / 2, kBloomHeight / 2);
				 });
	Graph.AddRenderPass("Bloom Blur")
		.Read(BloomArgs.Output5[0])
		.Write(&BloomArgs.Output5[1])
		.Execute([=](RenderGraphRegistry& Registry, D3D12CommandContext& Context)
				 {
					 D3D12Texture* Output5a = Registry.Get<D3D12Texture>(BloomArgs.Output5[0]);
					 struct Parameters
					 {
						 unsigned int InputIndex;
						 unsigned int OutputIndex;
					 } Args;
					 Args.InputIndex  = Registry.Get<D3D12ShaderResourceView>(BloomArgs.Output5Srvs[0])->GetIndex();
					 Args.OutputIndex = Registry.Get<D3D12UnorderedAccessView>(BloomArgs.Output5Uavs[1])->GetIndex();

					 D3D12ScopedEvent(Context, "Bloom Blur");
					 Context.SetPipelineState(Registry.GetPipelineState(PipelineStates::BloomBlur));
					 Context.SetComputeRootSignature(Registry.GetRootSignature(RootSignatures::BloomBlur));
					 Context->SetComputeRoot32BitConstants(0, 2, &Args, 0);
					 Context.Dispatch2D<8, 8>(Output5a->GetDesc().Width, Output5a->GetDesc().Height);
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

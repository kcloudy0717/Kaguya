#include "PostProcess.h"
#include "RHI/HlslResourceHandle.h"

struct BloomOutput
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

	[[nodiscard]] RHI::RgResourceHandle GetOutput() const noexcept
	{
		// Output1[1] contains final upsampled bloom texture
		return Output1[1];
	}

	[[nodiscard]] RHI::RgResourceHandle GetOutputSrv() const noexcept
	{
		return Output1Srvs[1];
	}

	[[nodiscard]] RHI::RgResourceHandle GetOutputUav() const noexcept
	{
		return Output1Uavs[1];
	}
};

PostProcess::PostProcess(ShaderCompiler* Compiler, RHI::D3D12Device* Device)
{
	BloomMaskShader			= Compiler->CompileCS(L"Shaders/PostProcess/Bloom/BloomMask.hlsl", ShaderCompileOptions(L"CSMain"));
	BloomDownsampleShader	= Compiler->CompileCS(L"Shaders/PostProcess/Bloom/BloomDownsample.hlsl", ShaderCompileOptions(L"CSMain"));
	BloomBlurShader			= Compiler->CompileCS(L"Shaders/PostProcess/Bloom/BloomBlur.hlsl", ShaderCompileOptions(L"CSMain"));
	BloomUpsampleBlurShader = Compiler->CompileCS(L"Shaders/PostProcess/Bloom/BloomUpsampleBlur.hlsl", ShaderCompileOptions(L"CSMain"));
	TonemapShader			= Compiler->CompileCS(L"Shaders/PostProcess/Tonemap.hlsl", ShaderCompileOptions(L"CSMain"));
	BayerDitherShader		= Compiler->CompileCS(L"Shaders/PostProcess/BayerDither.hlsl", ShaderCompileOptions(L"CSMain"));

	PostProcessRS = Device->CreateRootSignature(RHI::RootSignatureDesc().AddConstantBufferView(0, 0));
	{
		struct PsoStream
		{
			PipelineStateStreamRootSignature RootSignature;
			PipelineStateStreamCS			 CS;
		} Stream;
		Stream.RootSignature = &PostProcessRS;
		Stream.CS			 = &BloomMaskShader;

		BloomMaskPSO = Device->CreatePipelineState(L"BloomMask", Stream);
	}
	{
		struct PsoStream
		{
			PipelineStateStreamRootSignature RootSignature;
			PipelineStateStreamCS			 CS;
		} Stream;
		Stream.RootSignature = &PostProcessRS;
		Stream.CS			 = &BloomDownsampleShader;

		BloomDownsamplePSO = Device->CreatePipelineState(L"BloomDownsample", Stream);
	}
	{
		struct PsoStream
		{
			PipelineStateStreamRootSignature RootSignature;
			PipelineStateStreamCS			 CS;
		} Stream;
		Stream.RootSignature = &PostProcessRS;
		Stream.CS			 = &BloomBlurShader;

		BloomBlurPSO = Device->CreatePipelineState(L"BloomBlur", Stream);
	}
	{
		struct PsoStream
		{
			PipelineStateStreamRootSignature RootSignature;
			PipelineStateStreamCS			 CS;
		} Stream;
		Stream.RootSignature = &PostProcessRS;
		Stream.CS			 = &BloomUpsampleBlurShader;

		BloomUpsampleBlurPSO = Device->CreatePipelineState(L"Bloom Upsample Blur", Stream);
	}
	{
		struct PsoStream
		{
			PipelineStateStreamRootSignature RootSignature;
			PipelineStateStreamCS			 CS;
		} Stream;
		Stream.RootSignature = &PostProcessRS;
		Stream.CS			 = &TonemapShader;

		TonemapPSO = Device->CreatePipelineState(L"Tonemap", Stream);
	}
	{
		struct PsoStream
		{
			PipelineStateStreamRootSignature RootSignature;
			PipelineStateStreamCS			 CS;
		} Stream;
		Stream.RootSignature = &PostProcessRS;
		Stream.CS			 = &BayerDitherShader;

		BayerDitherPSO = Device->CreatePipelineState(L"Bayer Dither", Stream);
	}
}

PostProcessOutput PostProcess::Apply(RHI::RenderGraph& Graph, View View, PostProcessInput Input)
{
	assert(Input.Input.IsValid());
	assert(Input.InputSrv.IsValid());

	BloomOutput BloomOutput = {};
	if (EnableBloom)
	{
		constexpr DXGI_FORMAT Format	  = DXGI_FORMAT_R11G11B10_FLOAT;
		const uint32_t		  BloomWidth  = View.Width > 2560 ? 1280 : 640;
		const uint32_t		  BloomHeight = View.Height > 1440 ? 768 : 384;
		BloomOutput.Output1[0]			  = Graph.Create<RHI::D3D12Texture>(RHI::RgTextureDesc::Texture2D("Bloom Output 1a", Format, BloomWidth, BloomHeight, 1, {}, false, false, true));
		BloomOutput.Output1[1]			  = Graph.Create<RHI::D3D12Texture>(RHI::RgTextureDesc::Texture2D("Bloom Output 1b", Format, BloomWidth, BloomHeight, 1, {}, false, false, true));
		BloomOutput.Output2[0]			  = Graph.Create<RHI::D3D12Texture>(RHI::RgTextureDesc::Texture2D("Bloom Output 2a", Format, BloomWidth / 2, BloomHeight / 2, 1, {}, false, false, true));
		BloomOutput.Output2[1]			  = Graph.Create<RHI::D3D12Texture>(RHI::RgTextureDesc::Texture2D("Bloom Output 2b", Format, BloomWidth / 2, BloomHeight / 2, 1, {}, false, false, true));
		BloomOutput.Output3[0]			  = Graph.Create<RHI::D3D12Texture>(RHI::RgTextureDesc::Texture2D("Bloom Output 3a", Format, BloomWidth / 4, BloomHeight / 4, 1, {}, false, false, true));
		BloomOutput.Output3[1]			  = Graph.Create<RHI::D3D12Texture>(RHI::RgTextureDesc::Texture2D("Bloom Output 3b", Format, BloomWidth / 4, BloomHeight / 4, 1, {}, false, false, true));
		BloomOutput.Output4[0]			  = Graph.Create<RHI::D3D12Texture>(RHI::RgTextureDesc::Texture2D("Bloom Output 4a", Format, BloomWidth / 8, BloomHeight / 8, 1, {}, false, false, true));
		BloomOutput.Output4[1]			  = Graph.Create<RHI::D3D12Texture>(RHI::RgTextureDesc::Texture2D("Bloom Output 4b", Format, BloomWidth / 8, BloomHeight / 8, 1, {}, false, false, true));
		BloomOutput.Output5[0]			  = Graph.Create<RHI::D3D12Texture>(RHI::RgTextureDesc::Texture2D("Bloom Output 5a", Format, BloomWidth / 16, BloomHeight / 16, 1, {}, false, false, true));
		BloomOutput.Output5[1]			  = Graph.Create<RHI::D3D12Texture>(RHI::RgTextureDesc::Texture2D("Bloom Output 5b", Format, BloomWidth / 16, BloomHeight / 16, 1, {}, false, false, true));
		for (size_t i = 0; i < 2; ++i)
		{
			// Srvs
			BloomOutput.Output1Srvs[i] = Graph.Create<RHI::D3D12ShaderResourceView>(RHI::RgViewDesc().SetResource(BloomOutput.Output1[i]).AsTextureSrv());
			BloomOutput.Output2Srvs[i] = Graph.Create<RHI::D3D12ShaderResourceView>(RHI::RgViewDesc().SetResource(BloomOutput.Output2[i]).AsTextureSrv());
			BloomOutput.Output3Srvs[i] = Graph.Create<RHI::D3D12ShaderResourceView>(RHI::RgViewDesc().SetResource(BloomOutput.Output3[i]).AsTextureSrv());
			BloomOutput.Output4Srvs[i] = Graph.Create<RHI::D3D12ShaderResourceView>(RHI::RgViewDesc().SetResource(BloomOutput.Output4[i]).AsTextureSrv());
			BloomOutput.Output5Srvs[i] = Graph.Create<RHI::D3D12ShaderResourceView>(RHI::RgViewDesc().SetResource(BloomOutput.Output5[i]).AsTextureSrv());
			// Uavs
			BloomOutput.Output1Uavs[i] = Graph.Create<RHI::D3D12UnorderedAccessView>(RHI::RgViewDesc().SetResource(BloomOutput.Output1[i]).AsTextureUav());
			BloomOutput.Output2Uavs[i] = Graph.Create<RHI::D3D12UnorderedAccessView>(RHI::RgViewDesc().SetResource(BloomOutput.Output2[i]).AsTextureUav());
			BloomOutput.Output3Uavs[i] = Graph.Create<RHI::D3D12UnorderedAccessView>(RHI::RgViewDesc().SetResource(BloomOutput.Output3[i]).AsTextureUav());
			BloomOutput.Output4Uavs[i] = Graph.Create<RHI::D3D12UnorderedAccessView>(RHI::RgViewDesc().SetResource(BloomOutput.Output4[i]).AsTextureUav());
			BloomOutput.Output5Uavs[i] = Graph.Create<RHI::D3D12UnorderedAccessView>(RHI::RgViewDesc().SetResource(BloomOutput.Output5[i]).AsTextureUav());
		}

		Graph.AddRenderPass("Bloom Mask")
			.Read({ Input.Input })
			.Write({ &BloomOutput.Output1[0] })
			.Execute(
				[=, this](RHI::RenderGraphRegistry& Registry, RHI::D3D12CommandContext& Context)
				{
					struct Parameters
					{
						Math::Vec2f		InverseOutputSize;
						float			Threshold;
						HlslTexture2D	Input;
						HlslRWTexture2D Output;
					} Args;
					Args.InverseOutputSize = Math::Vec2f(1.0f / static_cast<float>(BloomWidth), 1.0f / static_cast<float>(BloomHeight));
					Args.Threshold		   = BloomThreshold;
					Args.Input			   = Registry.Get<RHI::D3D12ShaderResourceView>(Input.InputSrv);
					Args.Output			   = Registry.Get<RHI::D3D12UnorderedAccessView>(BloomOutput.Output1Uavs[0]);

					Context.SetPipelineState(&BloomMaskPSO);
					Context.SetComputeRootSignature(&PostProcessRS);
					Context.SetComputeConstantBuffer(0, Args);
					Context.Dispatch2D<8, 8>(BloomWidth, BloomHeight);
				});
		Graph.AddRenderPass("Bloom Downsample")
			.Read({ BloomOutput.Output1[0] })
			.Write({ &BloomOutput.Output2[0], &BloomOutput.Output3[0], &BloomOutput.Output4[0], &BloomOutput.Output5[0] })
			.Execute(
				[=, this](RHI::RenderGraphRegistry& Registry, RHI::D3D12CommandContext& Context)
				{
					struct Parameters
					{
						Math::Vec2f		InverseOutputSize;
						HlslTexture2D	Bloom;
						HlslRWTexture2D Output1;
						HlslRWTexture2D Output2;
						HlslRWTexture2D Output3;
						HlslRWTexture2D Output4;
					} Args;
					Args.InverseOutputSize = Math::Vec2f(1.0f / static_cast<float>(BloomWidth), 1.0f / static_cast<float>(BloomHeight));
					Args.Bloom			   = Registry.Get<RHI::D3D12ShaderResourceView>(BloomOutput.Output1Srvs[0]);
					Args.Output1		   = Registry.Get<RHI::D3D12UnorderedAccessView>(BloomOutput.Output2Uavs[0]);
					Args.Output2		   = Registry.Get<RHI::D3D12UnorderedAccessView>(BloomOutput.Output3Uavs[0]);
					Args.Output3		   = Registry.Get<RHI::D3D12UnorderedAccessView>(BloomOutput.Output4Uavs[0]);
					Args.Output4		   = Registry.Get<RHI::D3D12UnorderedAccessView>(BloomOutput.Output5Uavs[0]);

					Context.SetPipelineState(&BloomDownsamplePSO);
					Context.SetComputeRootSignature(&PostProcessRS);
					Context.SetComputeConstantBuffer(0, Args);
					Context.Dispatch2D<8, 8>(BloomWidth / 2, BloomHeight / 2);
				});
		Graph.AddRenderPass("Bloom Blur")
			.Read({ BloomOutput.Output5[0] })
			.Write({ &BloomOutput.Output5[1] })
			.Execute(
				[=, this](RHI::RenderGraphRegistry& Registry, RHI::D3D12CommandContext& Context)
				{
					RHI::D3D12Texture* Output5a = Registry.Get<RHI::D3D12Texture>(BloomOutput.Output5[0]);
					struct Parameters
					{
						HlslTexture2D	Input;
						HlslRWTexture2D Output;
					} Args;
					Args.Input	= Registry.Get<RHI::D3D12ShaderResourceView>(BloomOutput.Output5Srvs[0]);
					Args.Output = Registry.Get<RHI::D3D12UnorderedAccessView>(BloomOutput.Output5Uavs[1]);

					Context.SetPipelineState(&BloomBlurPSO);
					Context.SetComputeRootSignature(&PostProcessRS);
					Context.SetComputeConstantBuffer(0, Args);
					Context.Dispatch2D<8, 8>(static_cast<UINT>(Output5a->GetDesc().Width), Output5a->GetDesc().Height);
				});

		BlurUpsampleInputParameters UpsampleArgs;

		// 4
		UpsampleArgs.HighResInput	  = BloomOutput.Output4[0];
		UpsampleArgs.HighResOutput	  = &BloomOutput.Output4[1];
		UpsampleArgs.LowResInput	  = BloomOutput.Output5[1];
		UpsampleArgs.HighResInputSrv  = BloomOutput.Output4Srvs[0];
		UpsampleArgs.HighResOutputUav = BloomOutput.Output4Uavs[1];
		UpsampleArgs.LowResInputSrv	  = BloomOutput.Output5Srvs[1];
		BlurUpsample("Bloom Upsample 4", Graph, UpsampleArgs, 0.65f);
		// 3
		UpsampleArgs.HighResInput	  = BloomOutput.Output3[0];
		UpsampleArgs.HighResOutput	  = &BloomOutput.Output3[1];
		UpsampleArgs.LowResInput	  = BloomOutput.Output4[1];
		UpsampleArgs.HighResInputSrv  = BloomOutput.Output3Srvs[0];
		UpsampleArgs.HighResOutputUav = BloomOutput.Output3Uavs[1];
		UpsampleArgs.LowResInputSrv	  = BloomOutput.Output4Srvs[1];
		BlurUpsample("Bloom Upsample 3", Graph, UpsampleArgs, 0.65f);
		// 2
		UpsampleArgs.HighResInput	  = BloomOutput.Output2[0];
		UpsampleArgs.HighResOutput	  = &BloomOutput.Output2[1];
		UpsampleArgs.LowResInput	  = BloomOutput.Output3[1];
		UpsampleArgs.HighResInputSrv  = BloomOutput.Output2Srvs[0];
		UpsampleArgs.HighResOutputUav = BloomOutput.Output2Uavs[1];
		UpsampleArgs.LowResInputSrv	  = BloomOutput.Output3Srvs[1];
		BlurUpsample("Bloom Upsample 2", Graph, UpsampleArgs, 0.65f);
		// 1
		UpsampleArgs.HighResInput	  = BloomOutput.Output1[0];
		UpsampleArgs.HighResOutput	  = &BloomOutput.Output1[1];
		UpsampleArgs.LowResInput	  = BloomOutput.Output2[1];
		UpsampleArgs.HighResInputSrv  = BloomOutput.Output1Srvs[0];
		UpsampleArgs.HighResOutputUav = BloomOutput.Output1Uavs[1];
		UpsampleArgs.LowResInputSrv	  = BloomOutput.Output2Srvs[1];
		BlurUpsample("Bloom Upsample 1", Graph, UpsampleArgs, 0.65f);
	}

	PostProcessOutput PostProcessOutput;
	PostProcessOutput.Output	= Graph.Create<RHI::D3D12Texture>(RHI::RgTextureDesc::Texture2D("Tonemap Output", RHI::D3D12SwapChain::Format, View.Width, View.Height, 1, {}, false, false, true));
	PostProcessOutput.OutputSrv = Graph.Create<RHI::D3D12ShaderResourceView>(RHI::RgViewDesc().SetResource(PostProcessOutput.Output).AsTextureSrv());
	PostProcessOutput.OutputUav = Graph.Create<RHI::D3D12UnorderedAccessView>(RHI::RgViewDesc().SetResource(PostProcessOutput.Output).AsTextureUav());

	Graph.AddRenderPass("Tonemap")
		.Read({ Input.Input, BloomOutput.GetOutput() })
		.Write({ &PostProcessOutput.Output })
		.Execute(
			[=, this](RHI::RenderGraphRegistry& Registry, RHI::D3D12CommandContext& Context)
			{
				struct Parameters
				{
					Math::Vec2f		InverseOutputSize;
					float			BloomIntensity;
					HlslTexture2D	Input;
					HlslTexture2D	Bloom;
					HlslRWTexture2D Output;
				} Args;
				Args.InverseOutputSize = Math::Vec2f(1.0f / float(View.Width), 1.0f / float(View.Height));
				Args.BloomIntensity	   = BloomIntensity;
				Args.Input			   = Registry.Get<RHI::D3D12ShaderResourceView>(Input.InputSrv);
				Args.Output			   = Registry.Get<RHI::D3D12UnorderedAccessView>(PostProcessOutput.OutputUav);
				if (BloomOutput.GetOutput().IsValid())
				{
					Args.Bloom = Registry.Get<RHI::D3D12ShaderResourceView>(BloomOutput.GetOutputSrv());
				}

				Context.SetPipelineState(&TonemapPSO);
				Context.SetComputeRootSignature(&PostProcessRS);
				Context.SetComputeConstantBuffer(0, Args);
				Context.Dispatch2D<8, 8>(View.Width, View.Height);
			});

	if (EnableBayerDither)
	{
		Graph.AddRenderPass("Bayer Dither")
			.Read({ PostProcessOutput.Output })
			.Write({ &PostProcessOutput.Output })
			.Execute(
				[=, this](RHI::RenderGraphRegistry& Registry, RHI::D3D12CommandContext& Context)
				{
					UINT ThreadGroupCountX = View.Width, ThreadGroupCountY = View.Height;
					Context.CalculateDispatchArgument<8, 8, 1>(&ThreadGroupCountX, &ThreadGroupCountY, nullptr);

					struct Parameters
					{
						HlslRWTexture2D Output;
						Math::Vec2u		DispatchArgument;
					} Args;
					Args.Output			  = Registry.Get<RHI::D3D12UnorderedAccessView>(PostProcessOutput.OutputUav);
					Args.DispatchArgument = { ThreadGroupCountX, ThreadGroupCountY };

					Context.SetPipelineState(&BayerDitherPSO);
					Context.SetComputeRootSignature(&PostProcessRS);
					Context.SetComputeConstantBuffer(0, Args);
					Context.Dispatch2D<8, 8>(View.Width, View.Height);
				});
	}

	return PostProcessOutput;
}

void PostProcess::BlurUpsample(std::string_view Name, RHI::RenderGraph& Graph, BlurUpsampleInputParameters Inputs, float UpsampleInterpolationFactor)
{
	Graph.AddRenderPass(Name)
		.Read({ Inputs.HighResInput, Inputs.LowResInput })
		.Write({ Inputs.HighResOutput })
		.Execute(
			[=, this](RHI::RenderGraphRegistry& Registry, RHI::D3D12CommandContext& Context)
			{
				RHI::D3D12Texture* HighResInput = Registry.Get<RHI::D3D12Texture>(Inputs.HighResInput);
				struct Parameters
				{
					Math::Vec2f		InverseOutputSize;
					float			UpsampleInterpolationFactor;
					HlslTexture2D	HighRes;
					HlslTexture2D	LowRes;
					HlslRWTexture2D Output;
				} Args;
				Args.InverseOutputSize			 = Math::Vec2f(1.0f / static_cast<float>(HighResInput->GetDesc().Width), 1.0f / static_cast<float>(HighResInput->GetDesc().Height));
				Args.UpsampleInterpolationFactor = UpsampleInterpolationFactor;
				Args.HighRes					 = Registry.Get<RHI::D3D12ShaderResourceView>(Inputs.HighResInputSrv);
				Args.LowRes						 = Registry.Get<RHI::D3D12ShaderResourceView>(Inputs.LowResInputSrv);
				Args.Output						 = Registry.Get<RHI::D3D12UnorderedAccessView>(Inputs.HighResOutputUav);

				Context.SetPipelineState(&BloomUpsampleBlurPSO);
				Context.SetComputeRootSignature(&PostProcessRS);
				Context.SetComputeConstantBuffer(0, Args);
				Context.Dispatch2D<8, 8>(static_cast<UINT>(HighResInput->GetDesc().Width), HighResInput->GetDesc().Height);
			});
}

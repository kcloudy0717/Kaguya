#pragma once
#include <RenderCore/RenderCore.h>

struct Shaders
{
	static constexpr LPCWSTR g_VSEntryPoint = L"VSMain";
	static constexpr LPCWSTR g_MSEntryPoint = L"MSMain";
	static constexpr LPCWSTR g_PSEntryPoint = L"PSMain";
	static constexpr LPCWSTR g_CSEntryPoint = L"CSMain";

	// Vertex Shaders
	struct VS
	{
		inline static Shader FullScreenTriangle;
	};

	// Mesh Shaders
	struct MS
	{
	};

	// Pixel Shaders
	struct PS
	{
		inline static Shader ToneMap;
	};

	// Compute Shaders
	struct CS
	{
		inline static Shader EASU;
		inline static Shader RCAS;
	};

	static void Compile()
	{
		const auto& ExecutableDirectory = Application::ExecutableDirectory;

		// VS
		{
			VS::FullScreenTriangle = RenderCore::Compiler->CompileShader(
				EShaderType::Vertex,
				ExecutableDirectory / L"Shaders/FullScreenTriangle.hlsl",
				g_VSEntryPoint,
				{});
		}

		// PS
		{
			PS::ToneMap = RenderCore::Compiler->CompileShader(
				EShaderType::Pixel,
				ExecutableDirectory / L"Shaders/PostProcess/ToneMap.hlsl",
				g_PSEntryPoint,
				{});
		}

		// CS
		{
			CS::EASU = RenderCore::Compiler->CompileShader(
				EShaderType::Compute,
				ExecutableDirectory / L"Shaders/PostProcess/FSR.hlsl",
				g_CSEntryPoint,
				{
					{ L"SAMPLE_SLOW_FALLBACK", L"0" },
					{ L"SAMPLE_BILINEAR", L"0" },
					{ L"SAMPLE_EASU", L"1" },
					{ L"SAMPLE_RCAS", L"0" },
				});

			CS::RCAS = RenderCore::Compiler->CompileShader(
				EShaderType::Compute,
				ExecutableDirectory / L"Shaders/PostProcess/FSR.hlsl",
				g_CSEntryPoint,
				{
					{ L"SAMPLE_SLOW_FALLBACK", L"0" },
					{ L"SAMPLE_BILINEAR", L"0" },
					{ L"SAMPLE_EASU", L"0" },
					{ L"SAMPLE_RCAS", L"1" },
				});
		}
	}
};

struct Libraries
{
	inline static Library PathTrace;

	static void Compile()
	{
		const auto& ExecutableDirectory = Application::ExecutableDirectory;

		PathTrace = RenderCore::Compiler->CompileLibrary(ExecutableDirectory / L"Shaders/PathTrace.hlsl");
	}
};

struct RootSignatures
{
	inline static RenderResourceHandle Tonemap;
	inline static RenderResourceHandle FSR;

	static void Compile(RenderDevice& Device)
	{
		Tonemap = Device.CreateRootSignature(RenderCore::Device->CreateRootSignature(
			[](RootSignatureBuilder& Builder)
			{
				Builder.Add32BitConstants<0, 0>(1); // register(b0, space0)

				// PointClamp
				Builder.AddStaticSampler<0, 0>(D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 16);

				Builder.AllowResourceDescriptorHeapIndexing();
				Builder.AllowSampleDescriptorHeapIndexing();
			}));

		FSR = Device.CreateRootSignature(RenderCore::Device->CreateRootSignature(
			[](RootSignatureBuilder& Builder)
			{
				Builder.Add32BitConstants<0, 0>(2);
				Builder.AddConstantBufferView<1, 0>();

				Builder.AddStaticSampler<0, 0>(
					D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT,
					D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
					1);

				Builder.AllowResourceDescriptorHeapIndexing();
				Builder.AllowSampleDescriptorHeapIndexing();
			}));
	}
};

struct PipelineStates
{
	inline static RenderResourceHandle Tonemap;
	inline static RenderResourceHandle FSREASU;
	inline static RenderResourceHandle FSRRCAS;

	static void Compile(RenderDevice& Device)
	{
		{
			auto DepthStencilState									  = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
			DepthStencilState.DepthEnable							  = FALSE;
			D3D12_RT_FORMAT_ARRAY RTFormatArray						  = {};
			RTFormatArray.RTFormats[RTFormatArray.NumRenderTargets++] = D3D12SwapChain::Format_sRGB;

			struct PipelineStateStream
			{
				CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE		pRootSignature;
				CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY	PrimitiveTopologyType;
				CD3DX12_PIPELINE_STATE_STREAM_VS					VS;
				CD3DX12_PIPELINE_STATE_STREAM_PS					PS;
				CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL			DepthStencilState;
				CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
			} Stream;

			Stream.pRootSignature		 = Device.GetRootSignature(RootSignatures::Tonemap)->GetApiHandle();
			Stream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			Stream.VS					 = Shaders::VS::FullScreenTriangle;
			Stream.PS					 = Shaders::PS::ToneMap;
			Stream.DepthStencilState	 = DepthStencilState;
			Stream.RTVFormats			 = RTFormatArray;

			Tonemap = Device.CreatePipelineState(RenderCore::Device->CreatePipelineState(Stream));
		}
		{
			D3D12_COMPUTE_PIPELINE_STATE_DESC PSODesc = {};
			PSODesc.pRootSignature					  = Device.GetRootSignature(RootSignatures::FSR)->GetApiHandle();
			PSODesc.CS								  = Shaders::CS::EASU;

			FSREASU = Device.CreatePipelineState(RenderCore::Device->CreateComputePipelineState(PSODesc));
		}
		{
			D3D12_COMPUTE_PIPELINE_STATE_DESC PSODesc = {};
			PSODesc.pRootSignature					  = Device.GetRootSignature(RootSignatures::FSR)->GetApiHandle();
			PSODesc.CS								  = Shaders::CS::RCAS;

			FSRRCAS = Device.CreatePipelineState(RenderCore::Device->CreateComputePipelineState(PSODesc));
		}
	}
};

struct RaytracingPipelineStates
{
	// Symbols
	static constexpr LPCWSTR g_RayGeneration = L"RayGeneration";
	static constexpr LPCWSTR g_Miss			 = L"Miss";
	static constexpr LPCWSTR g_ShadowMiss	 = L"ShadowMiss";
	static constexpr LPCWSTR g_ClosestHit	 = L"ClosestHit";

	// HitGroup Exports
	static constexpr LPCWSTR g_HitGroupExport = L"Default";

	inline static ShaderIdentifier g_RayGenerationSID;
	inline static ShaderIdentifier g_MissSID;
	inline static ShaderIdentifier g_ShadowMissSID;
	inline static ShaderIdentifier g_DefaultSID;

	inline static RenderResourceHandle GlobalRS, LocalHitGroupRS;
	inline static RenderResourceHandle RTPSO;

	static void Compile(RenderDevice& Device)
	{
		GlobalRS = Device.CreateRootSignature(RenderCore::Device->CreateRootSignature(
			[](RootSignatureBuilder& Builder)
			{
				Builder.AddConstantBufferView<0, 0>(); // g_SystemConstants		b0 | space0

				Builder.AddShaderResourceView<0, 0>(); // g_Scene				t0 | space0
				Builder.AddShaderResourceView<1, 0>(); // g_Materials			t1 | space0
				Builder.AddShaderResourceView<2, 0>(); // g_Lights				t2 | space0

				Builder.AllowResourceDescriptorHeapIndexing();
				Builder.AllowSampleDescriptorHeapIndexing();

				Builder.AddStaticSampler<0, 0>(
					D3D12_FILTER_MIN_MAG_MIP_POINT,
					D3D12_TEXTURE_ADDRESS_MODE_WRAP,
					16); // g_SamplerPointWrap			s0 | space0;
				Builder.AddStaticSampler<1, 0>(
					D3D12_FILTER_MIN_MAG_MIP_POINT,
					D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
					16); // g_SamplerPointClamp			s1 | space0;
				Builder.AddStaticSampler<2, 0>(
					D3D12_FILTER_MIN_MAG_MIP_LINEAR,
					D3D12_TEXTURE_ADDRESS_MODE_WRAP,
					16); // g_SamplerLinearWrap			s2 | space0;
				Builder.AddStaticSampler<3, 0>(
					D3D12_FILTER_MIN_MAG_MIP_LINEAR,
					D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
					16); // g_SamplerLinearClamp			s3 | space0;
				Builder.AddStaticSampler<4, 0>(
					D3D12_FILTER_ANISOTROPIC,
					D3D12_TEXTURE_ADDRESS_MODE_WRAP,
					16); // g_SamplerAnisotropicWrap		s4 | space0;
				Builder.AddStaticSampler<5, 0>(
					D3D12_FILTER_ANISOTROPIC,
					D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
					16); // g_SamplerAnisotropicClamp	s5 | space0;
			}));

		LocalHitGroupRS = Device.CreateRootSignature(RenderCore::Device->CreateRootSignature(
			[](RootSignatureBuilder& Builder)
			{
				Builder.Add32BitConstants<0, 1>(1); // RootConstants	b0 | space1

				Builder.AddShaderResourceView<0, 1>(); // VertexBuffer		t0 | space1
				Builder.AddShaderResourceView<1, 1>(); // IndexBuffer		t1 | space1

				Builder.SetAsLocalRootSignature();
			}));

		RTPSO = Device.CreateRaytracingPipelineState(RenderCore::Device->CreateRaytracingPipelineState(
			[&](RaytracingPipelineStateBuilder& Builder)
			{
				Builder.AddLibrary(Libraries::PathTrace, { g_RayGeneration, g_Miss, g_ShadowMiss, g_ClosestHit });

				Builder.AddHitGroup(g_HitGroupExport, {}, g_ClosestHit, {});

				Builder.AddRootSignatureAssociation(
					Device.GetRootSignature(LocalHitGroupRS)->GetApiHandle(),
					{ g_HitGroupExport });

				Builder.SetGlobalRootSignature(Device.GetRootSignature(GlobalRS)->GetApiHandle());

				constexpr UINT PayloadSize = 12	  // p
											 + 4  // materialID
											 + 8  // uv
											 + 8  // Ng
											 + 8; // Ns

				Builder.SetRaytracingShaderConfig(PayloadSize, D3D12_BUILTIN_TRIANGLE_INTERSECTION_ATTRIBUTES);

				// +1 for Primary, +1 for Shadow
				Builder.SetRaytracingPipelineConfig(2);
			}));

		g_RayGenerationSID = Device.GetRaytracingPipelineState(RTPSO)->GetShaderIdentifier(g_RayGeneration);
		g_MissSID		   = Device.GetRaytracingPipelineState(RTPSO)->GetShaderIdentifier(g_Miss);
		g_ShadowMissSID	   = Device.GetRaytracingPipelineState(RTPSO)->GetShaderIdentifier(g_ShadowMiss);
		g_DefaultSID	   = Device.GetRaytracingPipelineState(RTPSO)->GetShaderIdentifier(g_HitGroupExport);
	}
};

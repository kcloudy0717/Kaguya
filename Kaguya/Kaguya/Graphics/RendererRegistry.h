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
		inline static Shader GBuffer;
	};

	// Mesh Shaders
	struct MS
	{
	};

	// Pixel Shaders
	struct PS
	{
		inline static Shader ToneMap;
		inline static Shader GBuffer;
	};

	// Compute Shaders
	struct CS
	{
		inline static Shader EASU;
		inline static Shader RCAS;

		inline static Shader IndirectCull;
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

			VS::GBuffer = RenderCore::Compiler->CompileShader(
				EShaderType::Vertex,
				ExecutableDirectory / L"Shaders/GBuffer.hlsl",
				L"VSMain",
				{});
		}

		// PS
		{
			PS::ToneMap = RenderCore::Compiler->CompileShader(
				EShaderType::Pixel,
				ExecutableDirectory / L"Shaders/ToneMap.hlsl",
				g_PSEntryPoint,
				{});

			PS::GBuffer = RenderCore::Compiler->CompileShader(
				EShaderType::Pixel,
				ExecutableDirectory / L"Shaders/GBuffer.hlsl",
				L"PSMain",
				{});
		}

		// CS
		{
			CS::EASU = RenderCore::Compiler->CompileShader(
				EShaderType::Compute,
				ExecutableDirectory / L"Shaders/FSR.hlsl",
				g_CSEntryPoint,
				{
					{ L"SAMPLE_SLOW_FALLBACK", L"0" },
					{ L"SAMPLE_BILINEAR", L"0" },
					{ L"SAMPLE_EASU", L"1" },
					{ L"SAMPLE_RCAS", L"0" },
				});

			CS::RCAS = RenderCore::Compiler->CompileShader(
				EShaderType::Compute,
				ExecutableDirectory / L"Shaders/FSR.hlsl",
				g_CSEntryPoint,
				{
					{ L"SAMPLE_SLOW_FALLBACK", L"0" },
					{ L"SAMPLE_BILINEAR", L"0" },
					{ L"SAMPLE_EASU", L"0" },
					{ L"SAMPLE_RCAS", L"1" },
				});

			CS::IndirectCull = RenderCore::Compiler->CompileShader(
				EShaderType::Compute,
				ExecutableDirectory / L"Shaders/IndirectCull.hlsl",
				g_CSEntryPoint,
				{});
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

struct RenderPasses
{
	inline static D3D12RenderPass TonemapRenderPass;

	inline static D3D12RenderPass GBufferRenderPass;

	static void Compile()
	{
		{
			RenderPassDesc Desc		= {};
			FLOAT		   Color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
			Desc.AddRenderTarget({ .Format	   = D3D12SwapChain::Format_sRGB,
								   .LoadOp	   = ELoadOp::Clear,
								   .StoreOp	   = EStoreOp::Store,
								   .ClearValue = CD3DX12_CLEAR_VALUE(D3D12SwapChain::Format, Color) });
			TonemapRenderPass = D3D12RenderPass(RenderCore::Device, Desc);
		}
		{
			RenderPassDesc Desc		= {};
			FLOAT		   Color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			Desc.AddRenderTarget({ .Format	   = DXGI_FORMAT_R32G32B32A32_FLOAT,
								   .LoadOp	   = ELoadOp::Clear,
								   .StoreOp	   = EStoreOp::Store,
								   .ClearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R32G32B32A32_FLOAT, Color) });
			Desc.AddRenderTarget({ .Format	   = DXGI_FORMAT_R32G32B32A32_FLOAT,
								   .LoadOp	   = ELoadOp::Clear,
								   .StoreOp	   = EStoreOp::Store,
								   .ClearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R32G32B32A32_FLOAT, Color) });
			Desc.AddRenderTarget({ .Format	   = DXGI_FORMAT_R16G16_FLOAT,
								   .LoadOp	   = ELoadOp::Clear,
								   .StoreOp	   = EStoreOp::Store,
								   .ClearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R16G16_FLOAT, Color) });
			Desc.SetDepthStencil({ .Format	   = RenderCore::DepthFormat,
								   .LoadOp	   = ELoadOp::Clear,
								   .StoreOp	   = EStoreOp::Store,
								   .ClearValue = CD3DX12_CLEAR_VALUE(RenderCore::DepthFormat, 1.0f, 0xFF) });
			GBufferRenderPass = D3D12RenderPass(RenderCore::Device, Desc);
		}
	}
};

struct RootSignatures
{
	inline static RenderResourceHandle Tonemap;
	inline static RenderResourceHandle FSR;

	inline static RenderResourceHandle GBuffer;
	inline static RenderResourceHandle IndirectCull;

	static void Compile(RenderDevice& Device)
	{
		Tonemap = Device.CreateRootSignature(RenderCore::Device->CreateRootSignature(
			[](RootSignatureBuilder& Builder)
			{
				Builder.Add32BitConstants<0, 0>(1); // register(b0, space0)

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

		GBuffer = Device.CreateRootSignature(RenderCore::Device->CreateRootSignature(
			[](RootSignatureBuilder& Builder)
			{
				Builder.Add32BitConstants<0, 0>(1);
				Builder.AddConstantBufferView<1, 0>();

				Builder.AddShaderResourceView<0, 0>();
				Builder.AddShaderResourceView<1, 0>();
				Builder.AddShaderResourceView<2, 0>();

				Builder.AllowInputLayout();
				Builder.AllowResourceDescriptorHeapIndexing();
				Builder.AllowSampleDescriptorHeapIndexing();
			}));

		IndirectCull = Device.CreateRootSignature(RenderCore::Device->CreateRootSignature(
			[](RootSignatureBuilder& Builder)
			{
				Builder.AddConstantBufferView<0, 0>();
				Builder.AddShaderResourceView<0, 0>();
				Builder.AddUnorderedAccessView<0, 0>();

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

	inline static RenderResourceHandle GBuffer;
	inline static RenderResourceHandle IndirectCull;

	static void Compile(RenderDevice& Device)
	{
		{
			DepthStencilState DepthStencilState;
			DepthStencilState.DepthEnable = false;

			struct PsoStream
			{
				PipelineStateStreamRootSignature	 RootSignature;
				PipelineStateStreamPrimitiveTopology PrimitiveTopologyType;
				PipelineStateStreamVS				 VS;
				PipelineStateStreamPS				 PS;
				PipelineStateStreamDepthStencilState DepthStencilState;
				PipelineStateStreamRenderPass		 RenderPass;
			} Stream;
			Stream.RootSignature		 = Device.GetRootSignature(RootSignatures::Tonemap);
			Stream.PrimitiveTopologyType = PrimitiveTopology::Triangle;
			Stream.VS					 = &Shaders::VS::FullScreenTriangle;
			Stream.PS					 = &Shaders::PS::ToneMap;
			Stream.DepthStencilState	 = DepthStencilState;
			Stream.RenderPass			 = &RenderPasses::TonemapRenderPass;

			Tonemap = Device.CreatePipelineState(RenderCore::Device->CreatePipelineState(Stream));
		}
		{
			struct PsoStream
			{
				PipelineStateStreamRootSignature RootSignature;
				PipelineStateStreamCS			 CS;
			} Stream;
			Stream.RootSignature = Device.GetRootSignature(RootSignatures::FSR);
			Stream.CS			 = &Shaders::CS::EASU;

			FSREASU = Device.CreatePipelineState(RenderCore::Device->CreatePipelineState(Stream));
		}
		{
			struct PsoStream
			{
				PipelineStateStreamRootSignature RootSignature;
				PipelineStateStreamCS			 CS;
			} Stream;
			Stream.RootSignature = Device.GetRootSignature(RootSignatures::FSR);
			Stream.CS			 = &Shaders::CS::RCAS;

			FSRRCAS = Device.CreatePipelineState(RenderCore::Device->CreatePipelineState(Stream));
		}
		{
			D3D12InputLayout InputLayout(3);
			InputLayout.AddVertexLayoutElement("POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0);
			InputLayout.AddVertexLayoutElement("TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0);
			InputLayout.AddVertexLayoutElement("NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0);

			DepthStencilState DepthStencilState;
			DepthStencilState.DepthEnable = true;

			struct PsoStream
			{
				PipelineStateStreamRootSignature	 RootSignature;
				PipelineStateStreamInputLayout		 InputLayout;
				PipelineStateStreamPrimitiveTopology PrimitiveTopologyType;
				PipelineStateStreamVS				 VS;
				PipelineStateStreamPS				 PS;
				PipelineStateStreamDepthStencilState DepthStencilState;
				PipelineStateStreamRenderPass		 RenderPass;
			} Stream;
			Stream.RootSignature		 = Device.GetRootSignature(RootSignatures::GBuffer);
			Stream.InputLayout			 = InputLayout;
			Stream.PrimitiveTopologyType = PrimitiveTopology::Triangle;
			Stream.VS					 = &Shaders::VS::GBuffer;
			Stream.PS					 = &Shaders::PS::GBuffer;
			Stream.DepthStencilState	 = DepthStencilState;
			Stream.RenderPass			 = &RenderPasses::GBufferRenderPass;

			GBuffer = Device.CreatePipelineState(RenderCore::Device->CreatePipelineState(Stream));
		}
		{
			struct PsoStream
			{
				PipelineStateStreamRootSignature RootSignature;
				PipelineStateStreamCS			 CS;
			} Stream;
			Stream.RootSignature = Device.GetRootSignature(RootSignatures::IndirectCull);
			Stream.CS			 = &Shaders::CS::IndirectCull;

			IndirectCull = Device.CreatePipelineState(RenderCore::Device->CreatePipelineState(Stream));
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

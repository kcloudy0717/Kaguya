#pragma once
#include <RenderCore/RenderCore.h>
#include "RenderGraph/RenderGraph.h"

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
		inline static Shader Meshlet;
	};

	// Pixel Shaders
	struct PS
	{
		inline static Shader GBuffer;

		inline static Shader Meshlet;
	};

	// Compute Shaders
	struct CS
	{
		inline static Shader IndirectCull;
		inline static Shader IndirectCullMeshShaders;
		inline static Shader BloomMask;
		inline static Shader BloomDownsample;
		inline static Shader BloomBlur;
		inline static Shader BloomUpsampleBlur;
		inline static Shader Tonemap;
		inline static Shader BayerDither;
		inline static Shader Sobol;
	};

	struct RTX
	{
		inline static Shader PathTrace;
	};

	static void Compile()
	{
		const auto& ExecutableDirectory = Application::ExecutableDirectory;

		{
			ShaderCompileOptions Options(g_CSEntryPoint);
			RTX::PathTrace = RenderCore::Compiler->CompileShader(
				RHI_SHADER_TYPE::Compute,
				ExecutableDirectory / L"Shaders/PathTrace1_1.hlsl",
				Options);
		}

		// VS
		{
			ShaderCompileOptions Options(g_VSEntryPoint);
			VS::FullScreenTriangle = RenderCore::Compiler->CompileShader(
				RHI_SHADER_TYPE::Vertex,
				ExecutableDirectory / L"Shaders/FullScreenTriangle.hlsl",
				Options);
		}
		{
			ShaderCompileOptions Options(g_VSEntryPoint);
			VS::GBuffer = RenderCore::Compiler->CompileShader(
				RHI_SHADER_TYPE::Vertex,
				ExecutableDirectory / L"Shaders/GBuffer.hlsl",
				Options);
		}

		// MS
		{
			ShaderCompileOptions Options(g_MSEntryPoint);
			MS::Meshlet = RenderCore::Compiler->CompileShader(
				RHI_SHADER_TYPE::Mesh,
				ExecutableDirectory / L"Shaders/Meshlet.ms.hlsl",
				Options);
		}

		// PS
		{
			ShaderCompileOptions Options(g_PSEntryPoint);
			PS::GBuffer = RenderCore::Compiler->CompileShader(
				RHI_SHADER_TYPE::Pixel,
				ExecutableDirectory / L"Shaders/GBuffer.hlsl",
				Options);
		}
		{
			ShaderCompileOptions Options(g_PSEntryPoint);
			PS::Meshlet = RenderCore::Compiler->CompileShader(
				RHI_SHADER_TYPE::Pixel,
				ExecutableDirectory / L"Shaders/Meshlet.ms.hlsl",
				Options);
		}

		// CS
		{
			ShaderCompileOptions Options(g_CSEntryPoint);
			CS::IndirectCull = RenderCore::Compiler->CompileShader(
				RHI_SHADER_TYPE::Compute,
				ExecutableDirectory / L"Shaders/IndirectCull.hlsl",
				Options);
		}
		{
			ShaderCompileOptions Options(g_CSEntryPoint);
			CS::IndirectCullMeshShaders = RenderCore::Compiler->CompileShader(
				RHI_SHADER_TYPE::Compute,
				ExecutableDirectory / L"Shaders/IndirectCullMeshShader.hlsl",
				Options);
		}
		{
			ShaderCompileOptions Options(g_CSEntryPoint);
			CS::BloomMask = RenderCore::Compiler->CompileShader(
				RHI_SHADER_TYPE::Compute,
				ExecutableDirectory / L"Shaders/Bloom/BloomMask.hlsl",
				Options);
		}
		{
			ShaderCompileOptions Options(g_CSEntryPoint);
			CS::BloomDownsample = RenderCore::Compiler->CompileShader(
				RHI_SHADER_TYPE::Compute,
				ExecutableDirectory / L"Shaders/Bloom/BloomDownsample.hlsl",
				Options);
		}
		{
			ShaderCompileOptions Options(g_CSEntryPoint);
			CS::BloomBlur = RenderCore::Compiler->CompileShader(
				RHI_SHADER_TYPE::Compute,
				ExecutableDirectory / L"Shaders/Bloom/BloomBlur.hlsl",
				Options);
		}
		{
			ShaderCompileOptions Options(g_CSEntryPoint);
			CS::BloomUpsampleBlur = RenderCore::Compiler->CompileShader(
				RHI_SHADER_TYPE::Compute,
				ExecutableDirectory / L"Shaders/Bloom/BloomUpsampleBlur.hlsl",
				Options);
		}
		{
			ShaderCompileOptions Options(g_CSEntryPoint);
			CS::Tonemap = RenderCore::Compiler->CompileShader(
				RHI_SHADER_TYPE::Compute,
				ExecutableDirectory / L"Shaders/PostprocessComposition.hlsl",
				Options);
		}
		{
			ShaderCompileOptions Options(g_CSEntryPoint);
			CS::BayerDither = RenderCore::Compiler->CompileShader(
				RHI_SHADER_TYPE::Compute,
				ExecutableDirectory / L"Shaders/BayerDither.hlsl",
				Options);
		}
		{
			ShaderCompileOptions Options(g_CSEntryPoint);
			CS::Sobol = RenderCore::Compiler->CompileShader(
				RHI_SHADER_TYPE::Compute,
				ExecutableDirectory / L"Shaders/Sobol.hlsl",
				Options);
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
	inline static RgResourceHandle GBuffer;
	inline static RgResourceHandle IndirectCull;

	inline static RgResourceHandle Meshlet;

	inline static RgResourceHandle BloomMask;
	inline static RgResourceHandle BloomDownsample;
	inline static RgResourceHandle BloomBlur;
	inline static RgResourceHandle BloomUpsampleBlur;
	inline static RgResourceHandle Tonemap;
	inline static RgResourceHandle BayerDither;
	inline static RgResourceHandle Sobol;

	struct RTX
	{
		inline static RgResourceHandle PathTrace;
	};

	static void Compile(RenderGraphRegistry& Registry)
	{
		RTX::PathTrace = Registry.CreateRootSignature(RenderCore::Device->CreateRootSignature(
			RootSignatureDesc()
				.AddConstantBufferView<0, 0>()				// g_SystemConstants	register(b0, space0)
				.AddRaytracingAccelerationStructure<0, 0>() // g_Scene				register(t0, space0)
				.AddShaderResourceView<1, 0>()				// g_Materials			register(t1, space0)
				.AddShaderResourceView<2, 0>()				// g_Lights				register(t2, space0)
				.AddShaderResourceView<3, 0>()				// g_Meshes				register(t3, space0)
				.AllowResourceDescriptorHeapIndexing()
				.AllowSampleDescriptorHeapIndexing()));

		GBuffer = Registry.CreateRootSignature(RenderCore::Device->CreateRootSignature(
			RootSignatureDesc()
				.Add32BitConstants<0, 0>(1)
				.AddConstantBufferView<1, 0>()
				.AddShaderResourceView<0, 0>()
				.AddShaderResourceView<1, 0>()
				.AddShaderResourceView<2, 0>()
				.AllowInputLayout()
				.AllowResourceDescriptorHeapIndexing()
				.AllowSampleDescriptorHeapIndexing()));

		IndirectCull = Registry.CreateRootSignature(RenderCore::Device->CreateRootSignature(
			RootSignatureDesc()
				.AddConstantBufferView<0, 0>()
				.AddShaderResourceView<0, 0>()
				.AddUnorderedAccessViewWithCounter<0, 0>()
				.AllowResourceDescriptorHeapIndexing()
				.AllowSampleDescriptorHeapIndexing()));

		Meshlet = Registry.CreateRootSignature(RenderCore::Device->CreateRootSignature(
			RootSignatureDesc()
				.Add32BitConstants<0, 0>(1)
				.AddShaderResourceView<0, 0>()
				.AddShaderResourceView<1, 0>()
				.AddShaderResourceView<2, 0>()
				.AddShaderResourceView<3, 0>()
				.AddConstantBufferView<0, 1>()
				.AddShaderResourceView<0, 1>()
				.AddShaderResourceView<1, 1>()
				.AllowResourceDescriptorHeapIndexing()
				.AllowSampleDescriptorHeapIndexing()));

		BloomMask = Registry.CreateRootSignature(
			RenderCore::Device->CreateRootSignature(
				RootSignatureDesc()
					.Add32BitConstants<0, 0>(5)));

		BloomDownsample = Registry.CreateRootSignature(
			RenderCore::Device->CreateRootSignature(
				RootSignatureDesc()
					.Add32BitConstants<0, 0>(7)));

		BloomBlur = Registry.CreateRootSignature(
			RenderCore::Device->CreateRootSignature(
				RootSignatureDesc()
					.Add32BitConstants<0, 0>(2)));

		BloomUpsampleBlur = Registry.CreateRootSignature(
			RenderCore::Device->CreateRootSignature(
				RootSignatureDesc()
					.Add32BitConstants<0, 0>(6)));

		Tonemap = Registry.CreateRootSignature(RenderCore::Device->CreateRootSignature(
			RootSignatureDesc()
				.Add32BitConstants<0, 0>(6)));

		BayerDither = Registry.CreateRootSignature(RenderCore::Device->CreateRootSignature(
			RootSignatureDesc()
				.Add32BitConstants<0, 0>(2)));

		Sobol = Registry.CreateRootSignature(RenderCore::Device->CreateRootSignature(
			RootSignatureDesc()
				.Add32BitConstants<0, 0>(2)));
	}
};

struct PipelineStates
{
	inline static RgResourceHandle GBuffer;
	inline static RgResourceHandle IndirectCull;
	inline static RgResourceHandle IndirectCullMeshShader;

	inline static RgResourceHandle Meshlet;

	inline static RgResourceHandle BloomMask;
	inline static RgResourceHandle BloomDownsample;
	inline static RgResourceHandle BloomBlur;
	inline static RgResourceHandle BloomUpsampleBlur;
	inline static RgResourceHandle Tonemap;
	inline static RgResourceHandle BayerDither;
	inline static RgResourceHandle Sobol;

	struct RTX
	{
		inline static RgResourceHandle PathTrace;
	};

	static void Compile(RenderGraphRegistry& Device)
	{
		{
			struct PsoStream
			{
				PipelineStateStreamRootSignature RootSignature;
				PipelineStateStreamCS			 CS;
			} Stream;
			Stream.RootSignature = Device.GetRootSignature(RootSignatures::RTX::PathTrace);
			Stream.CS			 = &Shaders::RTX::PathTrace;

			RTX::PathTrace = Device.CreatePipelineState(RenderCore::Device->CreatePipelineState(L"RTX::PathTrace", Stream));
		}

		{
			D3D12InputLayout InputLayout(3);
			InputLayout.AddVertexLayoutElement("POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0);
			InputLayout.AddVertexLayoutElement("TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0);
			InputLayout.AddVertexLayoutElement("NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0);

			DepthStencilState DepthStencilState;
			DepthStencilState.DepthEnable = true;

			RenderTargetState RenderTargetState;
			RenderTargetState.RTFormats[0]	   = DXGI_FORMAT_R32G32B32A32_FLOAT;
			RenderTargetState.RTFormats[1]	   = DXGI_FORMAT_R32G32B32A32_FLOAT;
			RenderTargetState.RTFormats[2]	   = DXGI_FORMAT_R16G16_FLOAT;
			RenderTargetState.NumRenderTargets = 3;
			RenderTargetState.DSFormat		   = DXGI_FORMAT_D32_FLOAT;

			struct PsoStream
			{
				PipelineStateStreamRootSignature	 RootSignature;
				PipelineStateStreamInputLayout		 InputLayout;
				PipelineStateStreamPrimitiveTopology PrimitiveTopologyType;
				PipelineStateStreamVS				 VS;
				PipelineStateStreamPS				 PS;
				PipelineStateStreamDepthStencilState DepthStencilState;
				PipelineStateStreamRenderTargetState RenderTargetState;
			} Stream;
			Stream.RootSignature		 = Device.GetRootSignature(RootSignatures::GBuffer);
			Stream.InputLayout			 = &InputLayout;
			Stream.PrimitiveTopologyType = RHI_PRIMITIVE_TOPOLOGY::Triangle;
			Stream.VS					 = &Shaders::VS::GBuffer;
			Stream.PS					 = &Shaders::PS::GBuffer;
			Stream.DepthStencilState	 = DepthStencilState;
			Stream.RenderTargetState	 = RenderTargetState;

			GBuffer = Device.CreatePipelineState(RenderCore::Device->CreatePipelineState(L"GBuffer", Stream));
		}
		{
			struct PsoStream
			{
				PipelineStateStreamRootSignature RootSignature;
				PipelineStateStreamCS			 CS;
			} Stream;
			Stream.RootSignature = Device.GetRootSignature(RootSignatures::IndirectCull);
			Stream.CS			 = &Shaders::CS::IndirectCull;

			IndirectCull = Device.CreatePipelineState(RenderCore::Device->CreatePipelineState(L"IndirectCull", Stream));
		}
		{
			struct PsoStream
			{
				PipelineStateStreamRootSignature RootSignature;
				PipelineStateStreamCS			 CS;
			} Stream;
			Stream.RootSignature = Device.GetRootSignature(RootSignatures::IndirectCull);
			Stream.CS			 = &Shaders::CS::IndirectCullMeshShaders;

			IndirectCullMeshShader = Device.CreatePipelineState(RenderCore::Device->CreatePipelineState(L"IndirectCullMeshShader", Stream));
		}
		{
			DepthStencilState DepthStencilState;
			DepthStencilState.DepthEnable = true;

			RenderTargetState RenderTargetState;
			RenderTargetState.RTFormats[0]	   = DXGI_FORMAT_R32G32B32A32_FLOAT;
			RenderTargetState.RTFormats[1]	   = DXGI_FORMAT_R32G32B32A32_FLOAT;
			RenderTargetState.RTFormats[2]	   = DXGI_FORMAT_R16G16_FLOAT;
			RenderTargetState.NumRenderTargets = 3;
			RenderTargetState.DSFormat		   = RenderCore::DepthFormat;

			struct PsoStream
			{
				PipelineStateStreamRootSignature	 RootSignature;
				PipelineStateStreamPrimitiveTopology PrimitiveTopologyType;
				PipelineStateStreamMS				 MS;
				PipelineStateStreamPS				 PS;
				PipelineStateStreamDepthStencilState DepthStencilState;
				PipelineStateStreamRenderTargetState RenderTargetState;
			} Stream;
			Stream.RootSignature		 = Device.GetRootSignature(RootSignatures::Meshlet);
			Stream.PrimitiveTopologyType = RHI_PRIMITIVE_TOPOLOGY::Triangle;
			Stream.MS					 = &Shaders::MS::Meshlet;
			Stream.PS					 = &Shaders::PS::Meshlet;
			Stream.DepthStencilState	 = DepthStencilState;
			Stream.RenderTargetState	 = RenderTargetState;

			Meshlet = Device.CreatePipelineState(RenderCore::Device->CreatePipelineState(L"Meshlet", Stream));
		}
		{
			struct PsoStream
			{
				PipelineStateStreamRootSignature RootSignature;
				PipelineStateStreamCS			 CS;
			} Stream;
			Stream.RootSignature = Device.GetRootSignature(RootSignatures::BloomMask);
			Stream.CS			 = &Shaders::CS::BloomMask;

			BloomMask = Device.CreatePipelineState(RenderCore::Device->CreatePipelineState(L"BloomMask", Stream));
		}
		{
			struct PsoStream
			{
				PipelineStateStreamRootSignature RootSignature;
				PipelineStateStreamCS			 CS;
			} Stream;
			Stream.RootSignature = Device.GetRootSignature(RootSignatures::BloomDownsample);
			Stream.CS			 = &Shaders::CS::BloomDownsample;

			BloomDownsample = Device.CreatePipelineState(RenderCore::Device->CreatePipelineState(L"BloomDownsample", Stream));
		}
		{
			struct PsoStream
			{
				PipelineStateStreamRootSignature RootSignature;
				PipelineStateStreamCS			 CS;
			} Stream;
			Stream.RootSignature = Device.GetRootSignature(RootSignatures::BloomBlur);
			Stream.CS			 = &Shaders::CS::BloomBlur;

			BloomBlur = Device.CreatePipelineState(RenderCore::Device->CreatePipelineState(L"BloomBlur", Stream));
		}
		{
			struct PsoStream
			{
				PipelineStateStreamRootSignature RootSignature;
				PipelineStateStreamCS			 CS;
			} Stream;
			Stream.RootSignature = Device.GetRootSignature(RootSignatures::BloomUpsampleBlur);
			Stream.CS			 = &Shaders::CS::BloomUpsampleBlur;

			BloomUpsampleBlur = Device.CreatePipelineState(RenderCore::Device->CreatePipelineState(L"Bloom Upsample Blur", Stream));
		}
		{
			struct PsoStream
			{
				PipelineStateStreamRootSignature RootSignature;
				PipelineStateStreamCS			 CS;
			} Stream;
			Stream.RootSignature = Device.GetRootSignature(RootSignatures::Tonemap);
			Stream.CS			 = &Shaders::CS::Tonemap;

			Tonemap = Device.CreatePipelineState(RenderCore::Device->CreatePipelineState(L"Tonemap", Stream));
		}
		{
			struct PsoStream
			{
				PipelineStateStreamRootSignature RootSignature;
				PipelineStateStreamCS			 CS;
			} Stream;
			Stream.RootSignature = Device.GetRootSignature(RootSignatures::BayerDither);
			Stream.CS			 = &Shaders::CS::BayerDither;

			BayerDither = Device.CreatePipelineState(RenderCore::Device->CreatePipelineState(L"Bayer Dither", Stream));
		}
		{
			struct PsoStream
			{
				PipelineStateStreamRootSignature RootSignature;
				PipelineStateStreamCS			 CS;
			} Stream;
			Stream.RootSignature = Device.GetRootSignature(RootSignatures::Sobol);
			Stream.CS			 = &Shaders::CS::Sobol;

			Sobol = Device.CreatePipelineState(RenderCore::Device->CreatePipelineState(L"Sobol", Stream));
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

	inline static RgResourceHandle GlobalRS, LocalHitGroupRS;
	inline static RgResourceHandle RTPSO;

	static void Compile(RenderGraphRegistry& Registry)
	{
		GlobalRS = Registry.CreateRootSignature(RenderCore::Device->CreateRootSignature(
			RootSignatureDesc()
				.AddConstantBufferView<0, 0>()				// g_SystemConstants	b0, space0
				.AddRaytracingAccelerationStructure<0, 0>() // g_Scene				t0, space0
				.AddShaderResourceView<1, 0>()				// g_Materials			t1, space0
				.AddShaderResourceView<2, 0>()				// g_Lights				t2, space0
				.AllowResourceDescriptorHeapIndexing()
				.AllowSampleDescriptorHeapIndexing()));

		LocalHitGroupRS = Registry.CreateRootSignature(RenderCore::Device->CreateRootSignature(
			RootSignatureDesc()
				.Add32BitConstants<0, 1>(1)	   // RootConstants		b0, space1
				.AddShaderResourceView<0, 1>() // VertexBuffer		t0, space1
				.AddShaderResourceView<1, 1>() // IndexBuffer		t1, space1
				.AsLocalRootSignature()));

		D3D12_SHADER_BYTECODE Bytecode = {};
		Bytecode.pShaderBytecode	   = Libraries::PathTrace.GetPointer();
		Bytecode.BytecodeLength		   = Libraries::PathTrace.GetSize();
		constexpr UINT PayloadSize	   = 12 // p
									 + 4	// materialID
									 + 8	// uv
									 + 8	// Ng
									 + 8;	// Ns
		// +1 for Primary, +1 for Shadow
		constexpr UINT MaxTraceRecursionDepth = 2;

		RTPSO			   = Registry.CreateRaytracingPipelineState(RenderCore::Device->CreateRaytracingPipelineState(
			 RaytracingPipelineStateDesc()
				 .AddLibrary(Bytecode, { g_RayGeneration, g_Miss, g_ShadowMiss, g_ClosestHit })
				 .AddHitGroup(g_HitGroupExport, {}, g_ClosestHit, {})
				 .AddRootSignatureAssociation(Registry.GetRootSignature(LocalHitGroupRS)->GetApiHandle(), { g_HitGroupExport })
				 .SetGlobalRootSignature(Registry.GetRootSignature(GlobalRS)->GetApiHandle())
				 .SetRaytracingShaderConfig(PayloadSize, D3D12_BUILTIN_TRIANGLE_INTERSECTION_ATTRIBUTES)
				 .SetRaytracingPipelineConfig(MaxTraceRecursionDepth)));
		g_RayGenerationSID = Registry.GetRaytracingPipelineState(RTPSO)->GetShaderIdentifier(g_RayGeneration);
		g_MissSID		   = Registry.GetRaytracingPipelineState(RTPSO)->GetShaderIdentifier(g_Miss);
		g_ShadowMissSID	   = Registry.GetRaytracingPipelineState(RTPSO)->GetShaderIdentifier(g_ShadowMiss);
		g_DefaultSID	   = Registry.GetRaytracingPipelineState(RTPSO)->GetShaderIdentifier(g_HitGroupExport);
	}
};

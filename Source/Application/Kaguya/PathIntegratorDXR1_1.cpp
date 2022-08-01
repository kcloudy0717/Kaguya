#include "PathIntegratorDXR1_1.h"
#include <imgui.h>

PathIntegratorDXR1_1::PathIntegratorDXR1_1(
	RHI::D3D12Device* Device,
	ShaderCompiler*	  Compiler)
	: Renderer(Device, Compiler)
	, PostProcess(Compiler, Device)
{
	PathTraceCS = Compiler->CompileCS(L"Shaders/PathTrace1_1.hlsl", ShaderCompileOptions(L"CSMain"));
	PathTraceRS = Device->CreateRootSignature(
		RHI::RootSignatureDesc()
			.AddConstantBufferView(0, 0)
			.AddRaytracingAccelerationStructure(0, 0)
			.AddShaderResourceView(1, 0)
			.AddShaderResourceView(2, 0)
			.AddShaderResourceView(3, 0));
	{
		struct PsoStream
		{
			PipelineStateStreamRootSignature RootSignature;
			PipelineStateStreamCS			 CS;
		} Stream;
		Stream.RootSignature = &PathTraceRS;
		Stream.CS			 = &PathTraceCS;
		PathTracePSO		 = Device->CreatePipelineState(L"PathTrace", Stream);
	}

	AccelerationStructure = RaytracingAccelerationStructure(Device, 1);
}

void PathIntegratorDXR1_1::RenderOptions()
{
	bool ResetPathIntegrator = false;

	if (ImGui::Button("Restore Defaults"))
	{
		Settings			= {};
		ResetPathIntegrator = true;
	}

	ResetPathIntegrator |= ImGui::Checkbox("Anti-aliasing", &Settings.Antialiasing);
	ResetPathIntegrator |= ImGui::SliderFloat("Sky Intensity", &Settings.SkyIntensity, 0.0f, 50.0f);
	ResetPathIntegrator |= ImGui::SliderScalar("Max Depth", ImGuiDataType_U32, &Settings.MaxDepth, &PathIntegratorSettings::MinimumDepth, &PathIntegratorSettings::MaximumDepth);
	ResetPathIntegrator |= ImGui::SliderScalar("Max Accumulation", ImGuiDataType_U32, &Settings.MaxAccumulation, &PathIntegratorSettings::MinimumAccumulation, &PathIntegratorSettings::MaximumAccumulation);
	ResetPathIntegrator |= ImGui::Checkbox("Enable Bloom", &PostProcess.EnableBloom);
	ImGui::SliderFloat("Bloom Threshold", &PostProcess.BloomThreshold, 0.0f, 50.0f);
	ImGui::SliderFloat("Bloom Intensity", &PostProcess.BloomIntensity, 0.0f, 50.0f);

	ImGui::Checkbox("Enable Bayer Dither", &PostProcess.EnableBayerDither);

	if (ResetPathIntegrator)
	{
		ResetAccumulation();
	}

	ImGui::Text("Num Temporal Samples: %u", NumTemporalSamples);
	ImGui::Text("Samples Per Pixel: %u", 4u);
}

void PathIntegratorDXR1_1::Render(World* World, WorldRenderView* WorldRenderView, RHI::D3D12CommandContext& Context)
{
	WorldRenderView->Update(World, &AccelerationStructure);
	if (World->WorldState & EWorldState::EWorldState_Update)
	{
		World->WorldState = EWorldState_Render;
		ResetAccumulation();
	}

	RHI::D3D12SyncHandle ASBuildSyncHandle;
	if (AccelerationStructure.IsValid())
	{
		RHI::D3D12CommandContext& AsyncCompute = Device->GetLinkedDevice()->GetComputeContext();
		AsyncCompute.Open();
		{
			D3D12ScopedEvent(AsyncCompute, "Acceleration Structure");
			AccelerationStructure.Build(AsyncCompute);
		}
		AsyncCompute.Close();

		ASBuildSyncHandle = AsyncCompute.Execute(false);

		AccelerationStructure.PostBuild(ASBuildSyncHandle);
	}

	Context.GetCommandQueue()->WaitForSyncHandle(ASBuildSyncHandle);

	RHI::RenderGraph Graph(Allocator, Registry);

	struct PathTraceParameters
	{
		RHI::RgResourceHandle Output;
		RHI::RgResourceHandle Srv;
		RHI::RgResourceHandle Uav;
	} PathTraceArgs;
	PathTraceArgs.Output = Graph.Create<RHI::D3D12Texture>(RHI::RgTextureDesc::Texture2D("Path Trace Output", DXGI_FORMAT_R32G32B32A32_FLOAT, WorldRenderView->View.Width, WorldRenderView->View.Height, 1, {}, false, false, true));
	PathTraceArgs.Srv	 = Graph.Create<RHI::D3D12ShaderResourceView>(RHI::RgViewDesc().SetResource(PathTraceArgs.Output).AsTextureSrv());
	PathTraceArgs.Uav	 = Graph.Create<RHI::D3D12UnorderedAccessView>(RHI::RgViewDesc().SetResource(PathTraceArgs.Output).AsTextureUav());
	Graph.AddRenderPass("Path Trace")
		.Write({ &PathTraceArgs.Output })
		.Execute([=, this](RHI::RenderGraphRegistry& Registry, RHI::D3D12CommandContext& Context)
				 {
					 if (NumTemporalSamples >= Settings.MaxAccumulation)
					 {
						 return;
					 }

					 _declspec(align(256)) struct GlobalConstants
					 {
						 Hlsl::Camera Camera;

						 // x, y = Resolution
						 // z, w = 1 / Resolution
						 DirectX::XMFLOAT4 Resolution;

						 unsigned int NumLights;
						 unsigned int TotalFrameCount;

						 unsigned int MaxDepth;
						 unsigned int NumAccumulatedSamples;

						 unsigned int RenderTarget;

						 float SkyIntensity;

						 DirectX::XMUINT2 Dimensions;
						 unsigned int	  AntiAliasing;
						 int			  Sky;
					 } g_GlobalConstants					 = {};
					 g_GlobalConstants.Camera				 = GetHLSLCameraDesc(*WorldRenderView->Camera);
					 g_GlobalConstants.Resolution			 = { float(WorldRenderView->View.Width), float(WorldRenderView->View.Height), 1.0f / float(WorldRenderView->View.Width), 1.0f / float(WorldRenderView->View.Height) };
					 g_GlobalConstants.NumLights			 = WorldRenderView->NumLights;
					 g_GlobalConstants.TotalFrameCount		 = FrameCounter++;
					 g_GlobalConstants.MaxDepth				 = Settings.MaxDepth;
					 g_GlobalConstants.NumAccumulatedSamples = NumTemporalSamples++;
					 g_GlobalConstants.RenderTarget			 = Registry.Get<RHI::D3D12UnorderedAccessView>(PathTraceArgs.Uav)->GetIndex();
					 g_GlobalConstants.SkyIntensity			 = Settings.SkyIntensity;
					 g_GlobalConstants.Dimensions			 = { WorldRenderView->View.Width, WorldRenderView->View.Height };
					 g_GlobalConstants.AntiAliasing			 = Settings.Antialiasing;
					 g_GlobalConstants.Sky					 = World->ActiveSkyLight->SRVIndex;

					 Context.SetPipelineState(&PathTracePSO);
					 Context.SetComputeRootSignature(&PathTraceRS);
					 Context.SetComputeConstantBuffer(0, sizeof(GlobalConstants), &g_GlobalConstants);
					 Context.SetComputeRaytracingAccelerationStructure(1, AccelerationStructure.GetShaderResourceView());
					 Context->SetComputeRootShaderResourceView(2, WorldRenderView->Materials.GetGpuVirtualAddress());
					 Context->SetComputeRootShaderResourceView(3, WorldRenderView->Lights.GetGpuVirtualAddress());
					 Context->SetComputeRootShaderResourceView(4, WorldRenderView->Meshes.GetGpuVirtualAddress());

					 Context.Dispatch2D<16, 16>(WorldRenderView->View.Width, WorldRenderView->View.Height);
					 Context.UAVBarrier(nullptr);
				 });

	PostProcessInput  PostProcessInput	= { .Input = PathTraceArgs.Output, .InputSrv = PathTraceArgs.Srv };
	PostProcessOutput PostProcessOutput = PostProcess.Apply(Graph, WorldRenderView->View, PostProcessInput);

	// After render graph execution, we need to read tonemap output as part of imgui pipeline that is not part of the graph, so graph will automatically apply
	// resource barrier transition for us
	Graph.GetEpiloguePass()
		.Read({ PostProcessOutput.Output });

	Graph.Execute(Context);

	// DgmlBuilder Builder("Render Graph");
	// Graph.ExportDgml(Builder);
	// Builder.SaveAs(Application::ExecutableDirectory / "RenderGraph.dgml");

	Viewport = reinterpret_cast<void*>(Registry.Get<RHI::D3D12ShaderResourceView>(PostProcessOutput.OutputSrv)->GetGpuHandle().ptr);
}

void PathIntegratorDXR1_1::ResetAccumulation()
{
	NumTemporalSamples = 0;
}

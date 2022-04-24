#include "PathIntegratorDXR1_1.h"
#include "RendererRegistry.h"

#include "Tonemap.h"
#include "Bloom.h"

static BloomSettings   g_Bloom;
static TonemapSettings g_Tonemap;

PathIntegratorDXR1_1::PathIntegratorDXR1_1(
	RHI::D3D12Device* Device,
	ShaderCompiler*	  Compiler)
	: Renderer(Device, Compiler)
{
	Shaders::Compile(Compiler);
	RootSignatures::Compile(Device, Registry);
	PipelineStates::Compile(Device, Registry);

	AccelerationStructure = RaytracingAccelerationStructure(Device, 1, World::MeshLimit);
}

void PathIntegratorDXR1_1::RenderOptions()
{
	bool ResetPathIntegrator = false;

	if (ImGui::Button("Restore Defaults"))
	{
		PathIntegratorState = {};
		ResetPathIntegrator = true;
	}

	ResetPathIntegrator |= ImGui::Checkbox("Anti-aliasing", &PathIntegratorState.Antialiasing);
	ResetPathIntegrator |= ImGui::SliderFloat("Sky Intensity", &PathIntegratorState.SkyIntensity, 0.0f, 50.0f);
	ResetPathIntegrator |= ImGui::SliderScalar(
		"Max Depth",
		ImGuiDataType_U32,
		&PathIntegratorState.MaxDepth,
		&PathIntegratorState::MinimumDepth,
		&PathIntegratorState::MaximumDepth);

	ImGui::SliderFloat("Bloom Threshold", &g_Bloom.Threshold, 0.0f, 50.0f);
	ImGui::SliderFloat("Bloom Intensity", &g_Tonemap.BloomIntensity, 0.0f, 50.0f);

	if (ResetPathIntegrator)
	{
		NumTemporalSamples = 0;
	}

	ImGui::Text("Num Temporal Samples: %u", NumTemporalSamples);
	ImGui::Text("Samples Per Pixel: %u", 4u);
}

void PathIntegratorDXR1_1::Render(World* World, WorldRenderView* WorldRenderView, RHI::D3D12CommandContext& Context)
{
	WorldRenderView->Update(World, &AccelerationStructure);
	if (World->WorldState & EWorldState::EWorldState_Update)
	{
		World->WorldState  = EWorldState_Render;
		NumTemporalSamples = 0;
	}

	RHI::D3D12SyncHandle ASBuildSyncHandle;
	if (AccelerationStructure.IsValid())
	{
		RHI::D3D12CommandContext& AsyncCompute = Device->GetLinkedDevice()->GetAsyncComputeCommandContext();
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
	PathTraceArgs.Output = Graph.Create<RHI::D3D12Texture>(RHI::RgTextureDesc("Path Trace Output").SetFormat(DXGI_FORMAT_R32G32B32A32_FLOAT).SetExtent(WorldRenderView->View.Width, WorldRenderView->View.Height, 1).SetAllowUnorderedAccess());
	PathTraceArgs.Srv	 = Graph.Create<RHI::D3D12ShaderResourceView>(RHI::RgViewDesc().SetResource(PathTraceArgs.Output).AsTextureSrv());
	PathTraceArgs.Uav	 = Graph.Create<RHI::D3D12UnorderedAccessView>(RHI::RgViewDesc().SetResource(PathTraceArgs.Output).AsTextureUav());

	Graph.AddRenderPass("Path Trace")
		.Write({ &PathTraceArgs.Output })
		.Execute(
			[=, this](RHI::RenderGraphRegistry& Registry, RHI::D3D12CommandContext& Context)
			{
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
					unsigned int	 AntiAliasing;
					int				 Sky;
				} g_GlobalConstants						= {};
				g_GlobalConstants.Camera				= GetHLSLCameraDesc(*WorldRenderView->Camera);
				g_GlobalConstants.Resolution			= { static_cast<float>(WorldRenderView->View.Width), static_cast<float>(WorldRenderView->View.Height), 1.0f / static_cast<float>(WorldRenderView->View.Width), 1.0f / static_cast<float>(WorldRenderView->View.Height) };
				g_GlobalConstants.NumLights				= WorldRenderView->NumLights;
				g_GlobalConstants.TotalFrameCount		= FrameCounter++;
				g_GlobalConstants.MaxDepth				= PathIntegratorState.MaxDepth;
				g_GlobalConstants.NumAccumulatedSamples = NumTemporalSamples++;
				g_GlobalConstants.RenderTarget			= Registry.Get<RHI::D3D12UnorderedAccessView>(PathTraceArgs.Uav)->GetIndex();
				g_GlobalConstants.SkyIntensity			= PathIntegratorState.SkyIntensity;
				g_GlobalConstants.Dimensions			= { WorldRenderView->View.Width, WorldRenderView->View.Height };
				g_GlobalConstants.AntiAliasing			= PathIntegratorState.Antialiasing;
				g_GlobalConstants.Sky					= World->ActiveSkyLight->SRVIndex;

				Context.SetPipelineState(Registry.GetPipelineState(PipelineStates::RTX::PathTrace));
				Context.SetComputeRootSignature(Registry.GetRootSignature(RootSignatures::RTX::PathTrace));
				Context.SetComputeConstantBuffer(0, sizeof(GlobalConstants), &g_GlobalConstants);
				Context->SetComputeRootDescriptorTable(1, AccelerationStructure.GetShaderResourceView().GetGpuHandle());
				Context->SetComputeRootShaderResourceView(2, WorldRenderView->Materials.GetGpuVirtualAddress());
				Context->SetComputeRootShaderResourceView(3, WorldRenderView->Lights.GetGpuVirtualAddress());
				Context->SetComputeRootShaderResourceView(4, WorldRenderView->Meshes.GetGpuVirtualAddress());

				Context.Dispatch2D<16, 16>(WorldRenderView->View.Width, WorldRenderView->View.Height);
				Context.UAVBarrier(nullptr);
			});

	BloomInputParameters BloomInputArgs = {};
	BloomInputArgs.Input				= PathTraceArgs.Output;
	BloomInputArgs.Srv					= PathTraceArgs.Srv;
	BloomParameters BloomArgs			= AddBloomPass(Graph, WorldRenderView->View, BloomInputArgs, g_Bloom);

	TonemapInputParameters TonemapInputArgs = {};
	TonemapInputArgs.Input					= PathTraceArgs.Output;
	TonemapInputArgs.Srv					= PathTraceArgs.Srv;
	TonemapInputArgs.BloomInput				= BloomArgs.Output1[1]; // Output1[1] contains final upsampled bloom texture
	TonemapInputArgs.BloomInputSrv			= BloomArgs.Output1Srvs[1];
	TonemapParameters TonemapArgs			= AddTonemapPass(Graph, WorldRenderView->View, TonemapInputArgs, g_Tonemap);

	// After render graph execution, we need to read tonemap output as part of imgui pipeline that is not part of the graph, so graph will automatically apply
	// resource barrier transition for us
	Graph.GetEpiloguePass()
		.Read({ TonemapArgs.Output });

	Graph.Execute(Context);

	//DgmlBuilder Builder("Render Graph");
	//Graph.ExportDgml(Builder);
	//Builder.SaveAs(Application::ExecutableDirectory / "RenderGraph.dgml");

	Viewport = reinterpret_cast<void*>(Registry.Get<RHI::D3D12ShaderResourceView>(TonemapArgs.Srv)->GetGpuHandle().ptr);
}

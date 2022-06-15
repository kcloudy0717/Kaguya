#include "PathIntegratorDXR1_0.h"
#include <imgui.h>

PathIntegratorDXR1_0::PathIntegratorDXR1_0(
	RHI::D3D12Device* Device,
	ShaderCompiler*	  Compiler)
	: Renderer(Device, Compiler)
	, PostProcess(Compiler, Device)
{
	// Symbols
	static constexpr LPCWSTR g_RayGeneration = L"RayGeneration";
	static constexpr LPCWSTR g_Miss			 = L"Miss";
	static constexpr LPCWSTR g_ShadowMiss	 = L"ShadowMiss";
	static constexpr LPCWSTR g_ClosestHit	 = L"ClosestHit";
	// HitGroup Exports
	static constexpr LPCWSTR g_HitGroupExport = L"Default";

	RTLibrary = Compiler->CompileLibrary(L"Shaders/PathTrace.hlsl");

	GlobalRS = Device->CreateRootSignature(
		RHI::RootSignatureDesc()
			.AddConstantBufferView(0, 0)
			.AddRaytracingAccelerationStructure(0, 0)
			.AddShaderResourceView(1, 0)
			.AddShaderResourceView(2, 0));

	LocalHitGroupRS = Device->CreateRootSignature(
		RHI::RootSignatureDesc()
			.Add32BitConstants(0, 1, 1)
			.AddShaderResourceView(0, 1)
			.AddShaderResourceView(1, 1)
			.AsLocalRootSignature());

	const D3D12_SHADER_BYTECODE Bytecode	= { .pShaderBytecode = RTLibrary.GetPointer(), .BytecodeLength = RTLibrary.GetSize() };
	constexpr UINT				PayloadSize = 12 // p
								 + 4			 // materialID
								 + 8			 // uv
								 + 8			 // Ng
								 + 8;			 // Ns
	// +1 for Primary, +1 for Shadow
	constexpr UINT MaxTraceRecursionDepth = 2;

	RTPSO = Device->CreateRaytracingPipelineState(
		RHI::RaytracingPipelineStateDesc()
			.AddLibrary(Bytecode, { g_RayGeneration, g_Miss, g_ShadowMiss, g_ClosestHit })
			.AddHitGroup(g_HitGroupExport, {}, g_ClosestHit, {})
			.AddRootSignatureAssociation(LocalHitGroupRS.GetApiHandle(), { g_HitGroupExport })
			.SetGlobalRootSignature(GlobalRS.GetApiHandle())
			.SetRaytracingShaderConfig(PayloadSize, KAGUYA_RHI_D3D12_BUILTIN_TRIANGLE_INTERSECTION_ATTRIBUTES)
			.SetRaytracingPipelineConfig(MaxTraceRecursionDepth));
	g_RayGenerationSID = RTPSO.GetShaderIdentifier(g_RayGeneration);
	g_MissSID		   = RTPSO.GetShaderIdentifier(g_Miss);
	g_ShadowMissSID	   = RTPSO.GetShaderIdentifier(g_ShadowMiss);
	g_DefaultSID	   = RTPSO.GetShaderIdentifier(g_HitGroupExport);

	AccelerationStructure = RaytracingAccelerationStructure(Device, 1);

	RayGenerationShaderTable = ShaderBindingTable.AddRayGenerationShaderTable<void>(1);
	RayGenerationShaderTable->AddShaderRecord(g_RayGenerationSID);

	MissShaderTable = ShaderBindingTable.AddMissShaderTable<void>(2);
	MissShaderTable->AddShaderRecord(g_MissSID);
	MissShaderTable->AddShaderRecord(g_ShadowMissSID);

	HitGroupShaderTable = ShaderBindingTable.AddHitGroupShaderTable<RootArgument>(World::MeshLimit);

	ShaderBindingTable.Generate(Device->GetLinkedDevice());
}

void PathIntegratorDXR1_0::RenderOptions()
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
	ResetPathIntegrator |= ImGui::Checkbox("Enable Bloom", &PostProcess.EnableBloom);
	ImGui::SliderFloat("Bloom Threshold", &PostProcess.BloomThreshold, 0.0f, 50.0f);
	ImGui::SliderFloat("Bloom Intensity", &PostProcess.BloomIntensity, 0.0f, 50.0f);

	if (ResetPathIntegrator)
	{
		NumTemporalSamples = 0;
	}

	ImGui::Text("Num Temporal Samples: %u", NumTemporalSamples);
	ImGui::Text("Samples Per Pixel: %u", 4u);
}

void PathIntegratorDXR1_0::Render(World* World, WorldRenderView* WorldRenderView, RHI::D3D12CommandContext& Context)
{
	WorldRenderView->Update(World, &AccelerationStructure);
	if (World->WorldState & EWorldState::EWorldState_Update)
	{
		World->WorldState  = EWorldState_Render;
		NumTemporalSamples = 0;
	}

	RHI::D3D12SyncHandle CopySyncHandle, ASBuildSyncHandle;
	if (AccelerationStructure.IsValid())
	{
		RHI::D3D12CommandContext& Copy = Device->GetLinkedDevice()->GetCopyContext();
		Copy.Open();
		{
			// Update shader table
			HitGroupShaderTable->Reset();
			for (size_t i = 0; i < AccelerationStructure.StaticMeshes.size(); ++i)
			{
				StaticMeshComponent* MeshComponent = AccelerationStructure.StaticMeshes[i];
				ID3D12Resource*		 VertexBuffer  = MeshComponent->Mesh->VertexResource.GetResource();
				ID3D12Resource*		 IndexBuffer   = MeshComponent->Mesh->IndexResource.GetResource();

				RHI::D3D12RaytracingShaderTable<RootArgument>::Record Record = {};
				Record.ShaderIdentifier										 = g_DefaultSID;
				Record.RootArguments.MaterialIndex							 = static_cast<UINT>(i);
				Record.RootArguments.Padding								 = 0xDEADBEEF;
				Record.RootArguments.VertexBuffer							 = VertexBuffer->GetGPUVirtualAddress();
				Record.RootArguments.IndexBuffer							 = IndexBuffer->GetGPUVirtualAddress();

				HitGroupShaderTable->AddShaderRecord(Record);
			}
			ShaderBindingTable.WriteToGpu(Copy.GetGraphicsCommandList());
		}
		Copy.Close();
		CopySyncHandle = Copy.Execute(false);

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

	Context.GetCommandQueue()->WaitForSyncHandle(CopySyncHandle);
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

					 Context.SetPipelineState(&RTPSO);
					 Context.SetComputeRootSignature(&GlobalRS);
					 Context.SetComputeConstantBuffer(0, sizeof(GlobalConstants), &g_GlobalConstants);
					 Context.SetComputeRaytracingAccelerationStructure(1, AccelerationStructure.GetShaderResourceView());
					 Context->SetComputeRootShaderResourceView(2, WorldRenderView->Materials.GetGpuVirtualAddress());
					 Context->SetComputeRootShaderResourceView(3, WorldRenderView->Lights.GetGpuVirtualAddress());

					 D3D12_DISPATCH_RAYS_DESC Desc = ShaderBindingTable.GetDesc(0, 0);
					 Desc.Width					   = WorldRenderView->View.Width;
					 Desc.Height				   = WorldRenderView->View.Height;
					 Desc.Depth					   = 1;

					 Context.DispatchRays(&Desc);
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

#include "PathIntegratorDXR1_0.h"
#include "RendererRegistry.h"

#include "Tonemap.h"

void PathIntegratorDXR1_0::Initialize()
{
	Shaders::Compile(Compiler);
	Libraries::Compile(Compiler);
	RootSignatures::Compile(Device, Registry);
	PipelineStates::Compile(Device, Registry);
	RaytracingPipelineStates::Compile(Device, Registry);

	AccelerationStructure = RaytracingAccelerationStructure(Device, 1, World::MeshLimit);
	AccelerationStructure.Initialize();

	Materials = RHI::D3D12Buffer(
		Device->GetDevice(),
		sizeof(Hlsl::Material) * World::MaterialLimit,
		sizeof(Hlsl::Material),
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_RESOURCE_FLAG_NONE);
	Materials.Initialize();
	pMaterial = Materials.GetCpuVirtualAddress<Hlsl::Material>();

	Lights = RHI::D3D12Buffer(
		Device->GetDevice(),
		sizeof(Hlsl::Light) * World::LightLimit,
		sizeof(Hlsl::Light),
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_RESOURCE_FLAG_NONE);
	Lights.Initialize();
	pLights = Lights.GetCpuVirtualAddress<Hlsl::Light>();

	RayGenerationShaderTable = ShaderBindingTable.AddRayGenerationShaderTable<void>(1);
	RayGenerationShaderTable->AddShaderRecord(RaytracingPipelineStates::g_RayGenerationSID);

	MissShaderTable = ShaderBindingTable.AddMissShaderTable<void>(2);
	MissShaderTable->AddShaderRecord(RaytracingPipelineStates::g_MissSID);
	MissShaderTable->AddShaderRecord(RaytracingPipelineStates::g_ShadowMissSID);

	HitGroupShaderTable = ShaderBindingTable.AddHitGroupShaderTable<RootArgument>(World::MeshLimit);

	ShaderBindingTable.Generate(Device->GetDevice());
}

void PathIntegratorDXR1_0::Destroy()
{
}

void PathIntegratorDXR1_0::Render(World* World, RHI::D3D12CommandContext& Context)
{
	bool ResetPathIntegrator = false;

	if (ImGui::Begin("Renderer"))
	{
		ImGui::Text("Path Integrator (DXR 1.0)");
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

		ImGui::Text("Num Temporal Samples: %u", NumTemporalSamples);
		ImGui::Text("Samples Per Pixel: %u", 4u);
	}
	ImGui::End();

	NumMaterials = NumLights = 0;
	AccelerationStructure.Reset();
	World->Registry.view<CoreComponent, StaticMeshComponent>().each(
		[&](CoreComponent& Core, StaticMeshComponent& StaticMeshComponent)
		{
			if (StaticMeshComponent.Mesh)
			{
				AccelerationStructure.AddInstance(Core.Transform, &StaticMeshComponent);
				pMaterial[NumMaterials++] = GetHLSLMaterialDesc(StaticMeshComponent.Material);
			}
		});
	World->Registry.view<CoreComponent, LightComponent>().each(
		[&](CoreComponent& Core, LightComponent& Light)
		{
			pLights[NumLights++] = GetHLSLLightDesc(Core.Transform, Light);
		});

	RHI::D3D12SyncHandle CopySyncHandle;
	if (AccelerationStructure.IsValid())
	{
		RHI::D3D12CommandContext& Copy = Device->GetDevice()->GetCopyContext1();
		Copy.Open();

		// Update shader table
		HitGroupShaderTable->Reset();
		for (auto [i, MeshRenderer] : enumerate(AccelerationStructure.StaticMeshes))
		{
			ID3D12Resource* VertexBuffer = MeshRenderer->Mesh->VertexResource.GetResource();
			ID3D12Resource* IndexBuffer	 = MeshRenderer->Mesh->IndexResource.GetResource();

			RHI::D3D12RaytracingShaderTable<RootArgument>::Record Record = {};
			Record.ShaderIdentifier									= RaytracingPipelineStates::g_DefaultSID;
			Record.RootArguments.MaterialIndex						= static_cast<UINT>(i);
			Record.RootArguments.Padding							= 0xDEADBEEF;
			Record.RootArguments.VertexBuffer						= VertexBuffer->GetGPUVirtualAddress();
			Record.RootArguments.IndexBuffer						= IndexBuffer->GetGPUVirtualAddress();

			HitGroupShaderTable->AddShaderRecord(Record);
		}

		ShaderBindingTable.WriteToGpu(Copy.GetGraphicsCommandList());

		Copy.Close();

		CopySyncHandle = Copy.Execute(false);
	}

	RHI::D3D12SyncHandle ASBuildSyncHandle;
	if (AccelerationStructure.IsValid())
	{
		RHI::D3D12CommandContext& AsyncCompute = Device->GetDevice()->GetAsyncComputeCommandContext();
		AsyncCompute.Open();
		{
			D3D12ScopedEvent(AsyncCompute, "Acceleration Structure");
			AccelerationStructure.Build(AsyncCompute);
		}
		AsyncCompute.Close();

		ASBuildSyncHandle = AsyncCompute.Execute(false);

		AccelerationStructure.PostBuild(ASBuildSyncHandle);
	}

	if (World->WorldState & EWorldState::EWorldState_Update)
	{
		World->WorldState	= EWorldState_Render;
		ResetPathIntegrator = true;
	}

	Context.GetCommandQueue()->WaitForSyncHandle(CopySyncHandle);
	Context.GetCommandQueue()->WaitForSyncHandle(ASBuildSyncHandle);

	if (ResetPathIntegrator)
	{
		NumTemporalSamples = 0;
	}

	RHI::RenderGraph Graph(Allocator, Registry);

	struct PathTraceParameters
	{
		RHI::RgResourceHandle Output;
		RHI::RgResourceHandle OutputSrv;
		RHI::RgResourceHandle OutputUav;
	} PathTraceArgs;
	PathTraceArgs.Output = Graph.Create<RHI::D3D12Texture>(
		"Path Trace Output",
		RHI::RgTextureDesc()
			.SetFormat(DXGI_FORMAT_R32G32B32A32_FLOAT)
			.SetExtent(View.Width, View.Height, 1)
			.AllowUnorderedAccess());
	PathTraceArgs.OutputSrv = Graph.Create<RHI::D3D12ShaderResourceView>(
		"Path Trace Output Srv",
		RHI::RgViewDesc()
			.SetResource(PathTraceArgs.Output)
			.AsTextureSrv());
	PathTraceArgs.OutputUav = Graph.Create<RHI::D3D12UnorderedAccessView>(
		"Path Trace Output Uav",
		RHI::RgViewDesc()
			.SetResource(PathTraceArgs.Output)
			.AsTextureUav());

	Graph.AddRenderPass(
			 "Path Trace")
		.Write(&PathTraceArgs.Output)
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
					 g_GlobalConstants.Camera				 = GetHLSLCameraDesc(*World->ActiveCamera);
					 g_GlobalConstants.Resolution			 = { static_cast<float>(View.Width), static_cast<float>(View.Height), 1.0f / static_cast<float>(View.Width), 1.0f / static_cast<float>(View.Height) };
					 g_GlobalConstants.NumLights			 = NumLights;
					 g_GlobalConstants.TotalFrameCount		 = FrameCounter++;
					 g_GlobalConstants.MaxDepth				 = PathIntegratorState.MaxDepth;
					 g_GlobalConstants.NumAccumulatedSamples = NumTemporalSamples++;
					 g_GlobalConstants.RenderTarget			 = Registry.Get<RHI::D3D12UnorderedAccessView>(PathTraceArgs.OutputUav)->GetIndex();
					 g_GlobalConstants.SkyIntensity			 = PathIntegratorState.SkyIntensity;
					 g_GlobalConstants.Dimensions			 = { View.Width, View.Height };
					 g_GlobalConstants.AntiAliasing			 = PathIntegratorState.Antialiasing;

					 Context.SetPipelineState(Registry.GetRaytracingPipelineState(RaytracingPipelineStates::RTPSO));
					 Context.SetComputeRootSignature(Registry.GetRootSignature(RaytracingPipelineStates::GlobalRS));
					 Context.SetComputeConstantBuffer(0, sizeof(GlobalConstants), &g_GlobalConstants);
					 Context->SetComputeRootDescriptorTable(1, AccelerationStructure.GetShaderResourceView().GetGpuHandle());
					 Context->SetComputeRootShaderResourceView(2, Materials.GetGpuVirtualAddress());
					 Context->SetComputeRootShaderResourceView(3, Lights.GetGpuVirtualAddress());

					 D3D12_DISPATCH_RAYS_DESC Desc = ShaderBindingTable.GetDesc(0, 0);
					 Desc.Width					   = View.Width;
					 Desc.Height				   = View.Height;
					 Desc.Depth					   = 1;

					 Context.DispatchRays(&Desc);
					 Context.UAVBarrier(nullptr);
				 });

	TonemapInputParameters TonemapInputArgs = {};
	TonemapInputArgs.Input					= PathTraceArgs.Output;
	TonemapInputArgs.Srv					= PathTraceArgs.OutputSrv;
	TonemapParameters TonemapArgs			= AddTonemapPass(Graph, View, TonemapInputArgs);

	// After render graph execution, we need to read tonemap output as part of imgui pipeline that is not part of the graph, so graph will automatically apply
	// resource barrier transition for us
	Graph.GetEpiloguePass()
		.Read(TonemapArgs.Output);

	Graph.Execute(Context);

	Viewport	  = reinterpret_cast<void*>(Registry.Get<RHI::D3D12ShaderResourceView>(TonemapArgs.Srv)->GetGpuHandle().ptr);
}

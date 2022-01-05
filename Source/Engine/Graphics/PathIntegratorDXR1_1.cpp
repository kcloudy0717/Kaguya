#include "PathIntegratorDXR1_1.h"
#include "RendererRegistry.h"

#include "Tonemap.h"
#include "Bloom.h"

void PathIntegratorDXR1_1::Initialize()
{
	Shaders::Compile();
	RootSignatures::Compile(Registry);
	PipelineStates::Compile(Registry);

	AccelerationStructure = RaytracingAccelerationStructure(1, World::MeshLimit);
	AccelerationStructure.Initialize();

	Materials = D3D12Buffer(
		RenderCore::Device->GetDevice(),
		sizeof(Hlsl::Material) * World::MaterialLimit,
		sizeof(Hlsl::Material),
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_RESOURCE_FLAG_NONE);
	Materials.Initialize();
	pMaterial = Materials.GetCpuVirtualAddress<Hlsl::Material>();

	Lights = D3D12Buffer(
		RenderCore::Device->GetDevice(),
		sizeof(Hlsl::Light) * World::LightLimit,
		sizeof(Hlsl::Light),
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_RESOURCE_FLAG_NONE);
	Lights.Initialize();
	pLights = Lights.GetCpuVirtualAddress<Hlsl::Light>();

	Meshes = D3D12Buffer(
		RenderCore::Device->GetDevice(),
		sizeof(Hlsl::Mesh) * World::MeshLimit,
		sizeof(Hlsl::Mesh),
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_RESOURCE_FLAG_NONE);
	Meshes.Initialize();
	pMeshes = Meshes.GetCpuVirtualAddress<Hlsl::Mesh>();

	HlslMeshes.resize(World::MeshLimit);
}

void PathIntegratorDXR1_1::Destroy()
{
}

void PathIntegratorDXR1_1::Render(World* World, D3D12CommandContext& Context)
{
	bool ResetPathIntegrator = false;

	if (ImGui::Begin("Renderer"))
	{
		ImGui::Text("Path Integrator (DXR 1.1)");
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

	NumMaterials = NumLights = NumMeshes = 0;
	AccelerationStructure.Reset();
	World->Registry.view<CoreComponent, StaticMeshComponent>().each(
		[&](CoreComponent& Core, StaticMeshComponent& StaticMesh)
		{
			if (StaticMesh.Mesh)
			{
				AccelerationStructure.AddInstance(Core.Transform, &StaticMesh);

				D3D12Buffer& VertexBuffer = StaticMesh.Mesh->VertexResource;
				D3D12Buffer& IndexBuffer  = StaticMesh.Mesh->IndexResource;

				D3D12_DRAW_INDEXED_ARGUMENTS DrawIndexedArguments = {};
				DrawIndexedArguments.IndexCountPerInstance		  = StaticMesh.Mesh->IndexCount;
				DrawIndexedArguments.InstanceCount				  = 1;
				DrawIndexedArguments.StartIndexLocation			  = 0;
				DrawIndexedArguments.BaseVertexLocation			  = 0;
				DrawIndexedArguments.StartInstanceLocation		  = 0;

				pMaterial[NumMaterials] = GetHLSLMaterialDesc(StaticMesh.Material);

				Hlsl::Mesh Mesh		   = GetHLSLMeshDesc(Core.Transform);
				Mesh.PreviousTransform = HlslMeshes[NumMeshes].Transform;

				Mesh.VertexBuffer = VertexBuffer.GetVertexBufferView();
				Mesh.IndexBuffer  = IndexBuffer.GetIndexBufferView();
				//Mesh.Meshlets			 = StaticMesh.Mesh->MeshletResource.GetGpuVirtualAddress();
				//Mesh.UniqueVertexIndices = StaticMesh.Mesh->UniqueVertexIndexResource.GetGpuVirtualAddress();
				//Mesh.PrimitiveIndices	 = StaticMesh.Mesh->PrimitiveIndexResource.GetGpuVirtualAddress();

				Mesh.MaterialIndex = NumMaterials;
				//Mesh.NumMeshlets   = StaticMesh.Mesh->MeshletCount;
				Mesh.VertexView = StaticMesh.Mesh->VertexView.GetIndex();
				Mesh.IndexView	= StaticMesh.Mesh->IndexView.GetIndex();

				Mesh.BoundingBox = StaticMesh.Mesh->BoundingBox;

				Mesh.DrawIndexedArguments = DrawIndexedArguments;

				HlslMeshes[NumMeshes] = Mesh;

				++NumMaterials;
				++NumMeshes;
			}
		});
	World->Registry.view<CoreComponent, LightComponent>().each(
		[&](CoreComponent& Core, LightComponent& Light)
		{
			pLights[NumLights++] = GetHLSLLightDesc(Core.Transform, Light);
		});
	std::memcpy(pMeshes, HlslMeshes.data(), sizeof(Hlsl::Mesh) * World::MeshLimit);

	D3D12SyncHandle ASBuildSyncHandle;
	if (AccelerationStructure.IsValid())
	{
		D3D12CommandContext& AsyncCompute = RenderCore::Device->GetDevice()->GetAsyncComputeCommandContext();
		AsyncCompute.OpenCommandList();
		{
			D3D12ScopedEvent(AsyncCompute, "Acceleration Structure");
			AccelerationStructure.Build(AsyncCompute);
		}
		AsyncCompute.CloseCommandList();

		ASBuildSyncHandle = AsyncCompute.Execute(false);

		AccelerationStructure.PostBuild(ASBuildSyncHandle);
	}

	if (World->WorldState & EWorldState::EWorldState_Update)
	{
		World->WorldState	= EWorldState_Render;
		ResetPathIntegrator = true;
	}

	Context.GetCommandQueue()->WaitForSyncHandle(ASBuildSyncHandle);

	if (ResetPathIntegrator)
	{
		NumTemporalSamples = 0;
	}

	RenderGraph Graph(Allocator, Registry);

	struct PathTraceParameters
	{
		RgResourceHandle Output;
		RgResourceHandle Srv;
		RgResourceHandle Uav;
	} PathTraceArgs;
	PathTraceArgs.Output = Graph.Create<D3D12Texture>("Path Trace Output", RgTextureDesc().SetFormat(DXGI_FORMAT_R32G32B32A32_FLOAT).SetExtent(View.Width, View.Height, 1).AllowUnorderedAccess());
	PathTraceArgs.Srv	 = Graph.Create<D3D12ShaderResourceView>("Path Trace Output Srv", RgViewDesc().SetResource(PathTraceArgs.Output).AsTextureSrv());
	PathTraceArgs.Uav	 = Graph.Create<D3D12UnorderedAccessView>("Path Trace Output Uav", RgViewDesc().SetResource(PathTraceArgs.Output).AsTextureUav());

	Graph.AddRenderPass("Path Trace")
		.Write(&PathTraceArgs.Output)
		.Execute([=, this](RenderGraphRegistry& Registry, D3D12CommandContext& Context)
				 {
					 D3D12ScopedEvent(Context, "Path Trace");

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
					 g_GlobalConstants.Camera				 = GetHLSLCameraDesc(*World->ActiveCamera);
					 g_GlobalConstants.Resolution			 = { static_cast<float>(View.Width), static_cast<float>(View.Height), 1.0f / static_cast<float>(View.Width), 1.0f / static_cast<float>(View.Height) };
					 g_GlobalConstants.NumLights			 = NumLights;
					 g_GlobalConstants.TotalFrameCount		 = FrameCounter++;
					 g_GlobalConstants.MaxDepth				 = PathIntegratorState.MaxDepth;
					 g_GlobalConstants.NumAccumulatedSamples = NumTemporalSamples++;
					 g_GlobalConstants.RenderTarget			 = Registry.Get<D3D12UnorderedAccessView>(PathTraceArgs.Uav)->GetIndex();
					 g_GlobalConstants.SkyIntensity			 = PathIntegratorState.SkyIntensity;
					 g_GlobalConstants.Dimensions			 = { View.Width, View.Height };
					 g_GlobalConstants.AntiAliasing			 = PathIntegratorState.Antialiasing;
					 g_GlobalConstants.Sky					 = World->ActiveSkyLight->SRVIndex;

					 Context.SetPipelineState(Registry.GetPipelineState(PipelineStates::RTX::PathTrace));
					 Context.SetComputeRootSignature(Registry.GetRootSignature(RootSignatures::RTX::PathTrace));
					 Context.SetComputeConstantBuffer(0, sizeof(GlobalConstants), &g_GlobalConstants);
					 Context->SetComputeRootDescriptorTable(1, AccelerationStructure.GetShaderResourceView().GetGpuHandle());
					 Context->SetComputeRootShaderResourceView(2, Materials.GetGpuVirtualAddress());
					 Context->SetComputeRootShaderResourceView(3, Lights.GetGpuVirtualAddress());
					 Context->SetComputeRootShaderResourceView(4, Meshes.GetGpuVirtualAddress());

					 Context.Dispatch2D<16, 16>(View.Width, View.Height);
					 Context.UAVBarrier(nullptr);
				 });

	BloomInputParameters BloomInputArgs = {};
	BloomInputArgs.Input				= PathTraceArgs.Output;
	BloomInputArgs.Srv					= PathTraceArgs.Srv;
	BloomParameters BloomArgs			= AddBloomPass(Graph, View, BloomInputArgs);

	TonemapInputParameters TonemapInputArgs = {};
	TonemapInputArgs.Input					= PathTraceArgs.Output;
	TonemapInputArgs.Srv					= PathTraceArgs.Srv;
	TonemapInputArgs.BloomInput				= BloomArgs.Output1[1]; // Output1[1] contains final upsampled bloom texture
	TonemapInputArgs.BloomInputSrv			= BloomArgs.Output1Srvs[1];
	TonemapParameters TonemapArgs			= AddTonemapPass(Graph, View, TonemapInputArgs);

	// After render graph execution, we need to read tonemap output as part of imgui pipeline that is not part of the graph, so graph will automatically apply
	// resource barrier transition for us
	Graph.GetEpiloguePass()
		.Read(TonemapArgs.Output);

	Graph.Execute(Context);

	static bool ExportDgml = true;
	if (ExportDgml)
	{
		ExportDgml = false;
		DgmlBuilder Builder("Render Graph");
		Graph.ExportDgml(Builder);
		Builder.SaveAs(Application::ExecutableDirectory / "RenderGraph.dgml");
	}

	Viewport = reinterpret_cast<void*>(Registry.Get<D3D12ShaderResourceView>(TonemapArgs.Srv)->GetGpuHandle().ptr);
}

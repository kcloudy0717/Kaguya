#include "PathIntegratorDXR1_1.h"
#include "RendererRegistry.h"

#include "Tonemap.h"
#include "Bloom.h"

static BloomSettings   g_Bloom;
static TonemapSettings g_Tonemap;

void PathIntegratorDXR1_1::Initialize()
{
	Shaders::Compile(Compiler);
	RootSignatures::Compile(Device, Registry);
	PipelineStates::Compile(Device, Registry);

	AccelerationStructure = RaytracingAccelerationStructure(Device, 1, World::MeshLimit);

	Materials = RHI::D3D12Buffer(
		Device->GetDevice(),
		sizeof(Hlsl::Material) * World::MaterialLimit,
		sizeof(Hlsl::Material),
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_RESOURCE_FLAG_NONE);
	pMaterial = Materials.GetCpuVirtualAddress<Hlsl::Material>();

	Lights = RHI::D3D12Buffer(
		Device->GetDevice(),
		sizeof(Hlsl::Light) * World::LightLimit,
		sizeof(Hlsl::Light),
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_RESOURCE_FLAG_NONE);
	pLights = Lights.GetCpuVirtualAddress<Hlsl::Light>();

	Meshes = RHI::D3D12Buffer(
		Device->GetDevice(),
		sizeof(Hlsl::Mesh) * World::MeshLimit,
		sizeof(Hlsl::Mesh),
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_RESOURCE_FLAG_NONE);
	pMeshes = Meshes.GetCpuVirtualAddress<Hlsl::Mesh>();

	HlslMeshes.resize(World::MeshLimit);
}

void PathIntegratorDXR1_1::Destroy()
{
}

void PathIntegratorDXR1_1::Render(World* World, RHI::D3D12CommandContext& Context)
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

		ImGui::SliderFloat("Bloom Threshold", &g_Bloom.Threshold, 0.0f, 50.0f);
		ImGui::SliderFloat("Bloom Intensity", &g_Tonemap.BloomIntensity, 0.0f, 50.0f);

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

				RHI::D3D12Buffer& VertexBuffer = StaticMesh.Mesh->VertexResource;
				RHI::D3D12Buffer& IndexBuffer  = StaticMesh.Mesh->IndexResource;

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
				// Mesh.Meshlets			 = StaticMesh.Mesh->MeshletResource.GetGpuVirtualAddress();
				// Mesh.UniqueVertexIndices = StaticMesh.Mesh->UniqueVertexIndexResource.GetGpuVirtualAddress();
				// Mesh.PrimitiveIndices	 = StaticMesh.Mesh->PrimitiveIndexResource.GetGpuVirtualAddress();

				Mesh.MaterialIndex = NumMaterials;
				// Mesh.NumMeshlets   = StaticMesh.Mesh->MeshletCount;
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

	Context.GetCommandQueue()->WaitForSyncHandle(ASBuildSyncHandle);

	if (ResetPathIntegrator)
	{
		NumTemporalSamples = 0;
	}

	RHI::RenderGraph Graph(Allocator, Registry);

	struct PathTraceParameters
	{
		RHI::RgResourceHandle Output;
		RHI::RgResourceHandle Srv;
		RHI::RgResourceHandle Uav;
	} PathTraceArgs;
	PathTraceArgs.Output = Graph.Create<RHI::D3D12Texture>(RHI::RgTextureDesc("Path Trace Output").SetFormat(DXGI_FORMAT_R32G32B32A32_FLOAT).SetExtent(View.Width, View.Height, 1).AllowUnorderedAccess());
	PathTraceArgs.Srv	 = Graph.Create<RHI::D3D12ShaderResourceView>(RHI::RgViewDesc().SetResource(PathTraceArgs.Output).AsTextureSrv());
	PathTraceArgs.Uav	 = Graph.Create<RHI::D3D12UnorderedAccessView>(RHI::RgViewDesc().SetResource(PathTraceArgs.Output).AsTextureUav());

	Graph.AddRenderPass("Path Trace")
		.Write(&PathTraceArgs.Output)
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
				g_GlobalConstants.Camera				= GetHLSLCameraDesc(*World->ActiveCamera);
				g_GlobalConstants.Resolution			= { static_cast<float>(View.Width), static_cast<float>(View.Height), 1.0f / static_cast<float>(View.Width), 1.0f / static_cast<float>(View.Height) };
				g_GlobalConstants.NumLights				= NumLights;
				g_GlobalConstants.TotalFrameCount		= FrameCounter++;
				g_GlobalConstants.MaxDepth				= PathIntegratorState.MaxDepth;
				g_GlobalConstants.NumAccumulatedSamples = NumTemporalSamples++;
				g_GlobalConstants.RenderTarget			= Registry.Get<RHI::D3D12UnorderedAccessView>(PathTraceArgs.Uav)->GetIndex();
				g_GlobalConstants.SkyIntensity			= PathIntegratorState.SkyIntensity;
				g_GlobalConstants.Dimensions			= { View.Width, View.Height };
				g_GlobalConstants.AntiAliasing			= PathIntegratorState.Antialiasing;
				g_GlobalConstants.Sky					= World->ActiveSkyLight->SRVIndex;

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
	BloomParameters BloomArgs			= AddBloomPass(Graph, View, BloomInputArgs, g_Bloom);

	TonemapInputParameters TonemapInputArgs = {};
	TonemapInputArgs.Input					= PathTraceArgs.Output;
	TonemapInputArgs.Srv					= PathTraceArgs.Srv;
	TonemapInputArgs.BloomInput				= BloomArgs.Output1[1]; // Output1[1] contains final upsampled bloom texture
	TonemapInputArgs.BloomInputSrv			= BloomArgs.Output1Srvs[1];
	TonemapParameters TonemapArgs			= AddTonemapPass(Graph, View, TonemapInputArgs, g_Tonemap);

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

	Viewport = reinterpret_cast<void*>(Registry.Get<RHI::D3D12ShaderResourceView>(TonemapArgs.Srv)->GetGpuHandle().ptr);
}

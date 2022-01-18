#include "DeferredRenderer.h"
#include "DeferredRenderer.h"

#include "RendererRegistry.h"

void DeferredRenderer::Initialize()
{
	Shaders::Compile(Compiler);
	RootSignatures::Compile(Device, Registry);
	PipelineStates::Compile(Device, Registry);

#if USE_MESH_SHADERS
	CommandSignatureDesc Builder(6, sizeof(CommandSignatureParams));
	Builder.AddConstant(0, 0, 1);
	Builder.AddShaderResourceView(1);
	Builder.AddShaderResourceView(2);
	Builder.AddShaderResourceView(3);
	Builder.AddShaderResourceView(4);
	Builder.AddDispatchMesh();
#else
	CommandSignatureDesc Builder(4, sizeof(CommandSignatureParams));
	Builder.AddConstant(0, 0, 1);
	Builder.AddVertexBufferView(0);
	Builder.AddIndexBufferView();
	Builder.AddDrawIndexed();
#endif

#if USE_MESH_SHADERS
	CommandSignature = D3D12CommandSignature(
		Device,
		Builder,
		Registry.GetRootSignature(RootSignatures::Meshlet)->GetApiHandle());
#else
	CommandSignature = D3D12CommandSignature(
		RenderCore::Device,
		Builder,
		Registry.GetRootSignature(RootSignatures::GBuffer)->GetApiHandle());
#endif

	IndirectCommandBuffer = D3D12Buffer(
		Device->GetDevice(),
		CommandBufferCounterOffset + sizeof(UINT64),
		sizeof(CommandSignatureParams),
		D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	UAV = D3D12UnorderedAccessView(Device->GetDevice(), &IndirectCommandBuffer, World::MeshLimit, CommandBufferCounterOffset);

	Materials = D3D12Buffer(
		Device->GetDevice(),
		sizeof(Hlsl::Material) * World::MaterialLimit,
		sizeof(Hlsl::Material),
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_RESOURCE_FLAG_NONE);
	Materials.Initialize();
	pMaterial = Materials.GetCpuVirtualAddress<Hlsl::Material>();

	Lights = D3D12Buffer(
		Device->GetDevice(),
		sizeof(Hlsl::Light) * World::LightLimit,
		sizeof(Hlsl::Light),
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_RESOURCE_FLAG_NONE);
	Lights.Initialize();
	pLights = Lights.GetCpuVirtualAddress<Hlsl::Light>();

	Meshes = D3D12Buffer(
		Device->GetDevice(),
		sizeof(Hlsl::Mesh) * World::MeshLimit,
		sizeof(Hlsl::Mesh),
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_RESOURCE_FLAG_NONE);
	Meshes.Initialize();
	pMeshes = Meshes.GetCpuVirtualAddress<Hlsl::Mesh>();

	StaticMeshes.reserve(World::MeshLimit);
	HlslMeshes.resize(World::MeshLimit);
}

void DeferredRenderer::Destroy()
{
	//DEBUG_RENDERER_SHUTDOWN();
}

void DeferredRenderer::Render(World* World, D3D12CommandContext& Context)
{
	if (ImGui::Begin("Renderer"))
	{
		ImGui::Text("Deferred Renderer");
		constexpr const char* View[] = { "Albedo", "Normal", "Motion", "Depth" };
		ImGui::Combo("View", &ViewMode, View, static_cast<int>(std::size(View)));
	}
	ImGui::End();

	StaticMeshes.clear();

	NumMaterials = NumLights = NumMeshes = 0;
	World->Registry.view<CoreComponent, StaticMeshComponent>().each(
		[&](CoreComponent& Core, StaticMeshComponent& StaticMesh)
		{
			if (StaticMesh.Mesh)
			{
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

				Mesh.VertexBuffer		 = VertexBuffer.GetVertexBufferView();
				Mesh.IndexBuffer		 = IndexBuffer.GetIndexBufferView();
				Mesh.Meshlets			 = StaticMesh.Mesh->MeshletResource.GetGpuVirtualAddress();
				Mesh.UniqueVertexIndices = StaticMesh.Mesh->UniqueVertexIndexResource.GetGpuVirtualAddress();
				Mesh.PrimitiveIndices	 = StaticMesh.Mesh->PrimitiveIndexResource.GetGpuVirtualAddress();

				Mesh.MaterialIndex = NumMaterials;
				Mesh.NumMeshlets   = StaticMesh.Mesh->MeshletCount;

				Mesh.BoundingBox = StaticMesh.Mesh->BoundingBox;

				Mesh.DrawIndexedArguments = DrawIndexedArguments;

				HlslMeshes[NumMeshes] = Mesh;

				StaticMeshes.push_back(&StaticMesh);

				// DEBUG_RENDERER_ADD_BOUNDINGBOX(Core.Transform, Mesh.BoundingBox, Vector3f(1.0f));

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

	if (World->WorldState & EWorldState::EWorldState_Update)
	{
		World->WorldState = EWorldState_Render;
	}

	_declspec(align(256)) struct GlobalConstants
	{
		Hlsl::Camera Camera;

		unsigned int NumMeshes;
		unsigned int NumLights;
	} g_GlobalConstants			= {};
	g_GlobalConstants.Camera	= GetHLSLCameraDesc(*World->ActiveCamera);
	g_GlobalConstants.NumMeshes = NumMeshes;
	g_GlobalConstants.NumLights = NumLights;

	// Work flow is as following:
	// Copy Queue -> Compute Queue -> Graphics Queue
	// Workloads are executed asynchronously

	D3D12CommandContext& Copy = Device->GetDevice()->GetCopyContext1();
	Copy.Open();
	{
		Copy.ResetCounter(&IndirectCommandBuffer, CommandBufferCounterOffset);
	}
	Copy.Close();
	D3D12SyncHandle CopySyncHandle = Copy.Execute(false);

	D3D12SyncHandle ComputeSyncHandle;
	if (NumMeshes > 0)
	{
		D3D12CommandContext& AsyncCompute = Device->GetDevice()->GetAsyncComputeCommandContext();
		AsyncCompute.GetCommandQueue()->WaitForSyncHandle(CopySyncHandle);

		AsyncCompute.Open();
		{
#if USE_MESH_SHADERS
			AsyncCompute.SetPipelineState(Registry.GetPipelineState(PipelineStates::IndirectCullMeshShader));
#else
			AsyncCompute.SetPipelineState(Registry.GetPipelineState(PipelineStates::IndirectCull));
#endif
			AsyncCompute.SetComputeRootSignature(Registry.GetRootSignature(RootSignatures::IndirectCull));

			AsyncCompute.SetComputeConstantBuffer(0, sizeof(GlobalConstants), &g_GlobalConstants);
			AsyncCompute->SetComputeRootShaderResourceView(1, Meshes.GetGpuVirtualAddress());
			AsyncCompute->SetComputeRootDescriptorTable(2, UAV.GetGpuHandle());

			AsyncCompute.Dispatch1D<128>(NumMeshes);
		}
		AsyncCompute.Close();
		ComputeSyncHandle = AsyncCompute.Execute(false);
	}

	Context.GetCommandQueue()->WaitForSyncHandle(ComputeSyncHandle);

	RenderGraph Graph(Allocator, Registry);

	struct GBufferParameters
	{
		RgResourceHandle Albedo;
		RgResourceHandle Normal;
		RgResourceHandle Motion;
		RgResourceHandle Depth;
		RgResourceHandle RenderTarget;

		RgResourceHandle AlbedoSrv;
		RgResourceHandle NormalSrv;
		RgResourceHandle MotionSrv;
		RgResourceHandle DepthSrv;
	} GBufferArgs;
	FLOAT Color[]	   = { 0, 0, 0, 0 };
	GBufferArgs.Albedo = Graph.Create<D3D12Texture>(
		"Albedo",
		RgTextureDesc()
			.SetFormat(DXGI_FORMAT_R32G32B32A32_FLOAT)
			.SetExtent(View.Width, View.Height, 1)
			.AllowRenderTarget()
			.SetClearValue(CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R32G32B32A32_FLOAT, Color)));
	GBufferArgs.Normal = Graph.Create<D3D12Texture>(
		"Normal",
		RgTextureDesc()
			.SetFormat(DXGI_FORMAT_R32G32B32A32_FLOAT)
			.SetExtent(View.Width, View.Height, 1)
			.AllowRenderTarget()
			.SetClearValue(CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R32G32B32A32_FLOAT, Color)));
	GBufferArgs.Motion = Graph.Create<D3D12Texture>(
		"Motion",
		RgTextureDesc()
			.SetFormat(DXGI_FORMAT_R16G16_FLOAT)
			.SetExtent(View.Width, View.Height, 1)
			.AllowRenderTarget()
			.SetClearValue(CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R16G16_FLOAT, Color)));
	GBufferArgs.Depth = Graph.Create<D3D12Texture>(
		"Depth",
		RgTextureDesc()
			.SetFormat(DXGI_FORMAT_D32_FLOAT)
			.SetExtent(View.Width, View.Height, 1)
			.AllowDepthStencil()
			.SetClearValue(CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 1.0f, 0xFF)));
	GBufferArgs.RenderTarget = Graph.Create<D3D12RenderTarget>(
		"GBuffer",
		RgRenderTargetDesc()
			.AddRenderTarget(GBufferArgs.Albedo, false)
			.AddRenderTarget(GBufferArgs.Normal, false)
			.AddRenderTarget(GBufferArgs.Motion, false)
			.SetDepthStencil(GBufferArgs.Depth));

	GBufferArgs.AlbedoSrv = Graph.Create<D3D12ShaderResourceView>("Albedo Srv", RgViewDesc().SetResource(GBufferArgs.Albedo).AsTextureSrv());
	GBufferArgs.NormalSrv = Graph.Create<D3D12ShaderResourceView>("Normal Srv", RgViewDesc().SetResource(GBufferArgs.Normal).AsTextureSrv());
	GBufferArgs.MotionSrv = Graph.Create<D3D12ShaderResourceView>("Motion Srv", RgViewDesc().SetResource(GBufferArgs.Motion).AsTextureSrv());
	GBufferArgs.DepthSrv  = Graph.Create<D3D12ShaderResourceView>("Depth Srv", RgViewDesc().SetResource(GBufferArgs.Depth).AsTextureSrv());

#if USE_MESH_SHADERS
	Graph.AddRenderPass("GBuffer (Mesh Shaders)")
		.Write(&GBufferArgs.Albedo)
		.Write(&GBufferArgs.Normal)
		.Write(&GBufferArgs.Motion)
		.Write(&GBufferArgs.Depth)
		.Execute([=, this](RenderGraphRegistry& Registry, D3D12CommandContext& Context)
				 {
					 D3D12ScopedEvent(Context, "GBuffer (Mesh Shaders)");

					 Context.SetPipelineState(Registry.GetPipelineState(PipelineStates::Meshlet));
					 Context.SetGraphicsRootSignature(Registry.GetRootSignature(RootSignatures::Meshlet));
					 Context.SetGraphicsConstantBuffer(5, sizeof(GlobalConstants), &g_GlobalConstants);
					 Context->SetGraphicsRootShaderResourceView(6, Materials.GetGpuVirtualAddress());
					 Context->SetGraphicsRootShaderResourceView(7, Meshes.GetGpuVirtualAddress());

					 Context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
					 Context.SetViewport(RHIViewport(0.0f, 0.0f, View.Width, View.Height, 0.0f, 1.0f));
					 Context.SetScissorRect(RHIRect(0, 0, View.Width, View.Height));

					 D3D12RenderTarget* RenderTarget = Registry.Get<D3D12RenderTarget>(GBufferArgs.RenderTarget);
					 Context.ClearRenderTarget(RenderTarget);
					 Context.SetRenderTarget(RenderTarget);

					 Context->ExecuteIndirect(
						 CommandSignature,
						 World::MeshLimit,
						 IndirectCommandBuffer.GetResource(),
						 0,
						 IndirectCommandBuffer.GetResource(),
						 CommandBufferCounterOffset);

					 // DEBUG_RENDERER_RENDER(g_GlobalConstants.Camera.ViewProjection, Context);
				 });
#else
	Graph.AddRenderPass("GBuffer")
		.Write(&GBufferArgs.Albedo)
		.Write(&GBufferArgs.Normal)
		.Write(&GBufferArgs.Motion)
		.Write(&GBufferArgs.Depth)
		.Execute([=, this](RenderGraphRegistry& Registry, D3D12CommandContext& Context)
				 {
					 D3D12ScopedEvent(Context, "GBuffer");

					 Context.SetPipelineState(Registry.GetPipelineState(PipelineStates::GBuffer));
					 Context.SetGraphicsRootSignature(Registry.GetRootSignature(RootSignatures::GBuffer));
					 Context.SetGraphicsConstantBuffer(1, sizeof(GlobalConstants), &g_GlobalConstants);
					 Context->SetGraphicsRootShaderResourceView(2, Materials.GetGpuVirtualAddress());
					 Context->SetGraphicsRootShaderResourceView(3, Lights.GetGpuVirtualAddress());
					 Context->SetGraphicsRootShaderResourceView(4, Meshes.GetGpuVirtualAddress());

					 Context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
					 Context.SetViewport(RHIViewport(0.0f, 0.0f, View.Width, View.Height, 0.0f, 1.0f));
					 Context.SetScissorRect(RHIRect(0, 0, View.Width, View.Height));

					 D3D12RenderTarget* RenderTarget = Registry.Get<D3D12RenderTarget>(GBufferArgs.RenderTarget);
					 Context.ClearRenderTarget(RenderTarget);
					 Context.SetRenderTarget(RenderTarget);

					 Context->ExecuteIndirect(
						 CommandSignature,
						 World::MeshLimit,
						 IndirectCommandBuffer.GetResource(),
						 0,
						 IndirectCommandBuffer.GetResource(),
						 CommandBufferCounterOffset);

					 // DEBUG_RENDERER_RENDER(g_GlobalConstants.Camera.ViewProjection, Context);
				 });
#endif //

	RgResourceHandle Views[] = {
		GBufferArgs.AlbedoSrv,
		GBufferArgs.NormalSrv,
		GBufferArgs.MotionSrv,
		GBufferArgs.DepthSrv,
	};

	Graph.GetEpiloguePass()
		.Read(GBufferArgs.Albedo)
		.Read(GBufferArgs.Normal)
		.Read(GBufferArgs.Motion)
		.Read(GBufferArgs.Depth);

	Graph.Execute(Context);
	Viewport = reinterpret_cast<void*>(Registry.Get<D3D12ShaderResourceView>(Views[ViewMode])->GetGpuHandle().ptr);
}

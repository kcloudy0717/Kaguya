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
	RHI::CommandSignatureDesc Builder(4, sizeof(CommandSignatureParams));
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
	CommandSignature = RHI::D3D12CommandSignature(
		Device,
		Builder,
		Registry.GetRootSignature(RootSignatures::GBuffer)->GetApiHandle());
#endif

	IndirectCommandBuffer = RHI::D3D12Buffer(
		Device->GetDevice(),
		CommandBufferCounterOffset + sizeof(UINT64),
		sizeof(CommandSignatureParams),
		D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	UAV = RHI::D3D12UnorderedAccessView(Device->GetDevice(), &IndirectCommandBuffer, World::MeshLimit, CommandBufferCounterOffset);

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

	StaticMeshes.reserve(World::MeshLimit);
	HlslMeshes.resize(World::MeshLimit);
}

void DeferredRenderer::Destroy()
{
}

void DeferredRenderer::Render(World* World, RHI::D3D12CommandContext& Context)
{
	if (ImGui::Begin("Renderer"))
	{
		ImGui::Text("Deferred Renderer");
		constexpr const char* View[] = { "Normal", "Material Id", "Motion", "Depth" };
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
				if (StaticMesh.Mesh->Options.GenerateMeshlets)
				{
					Mesh.Meshlets			 = StaticMesh.Mesh->MeshletResource.GetGpuVirtualAddress();
					Mesh.UniqueVertexIndices = StaticMesh.Mesh->UniqueVertexIndexResource.GetGpuVirtualAddress();
					Mesh.PrimitiveIndices	 = StaticMesh.Mesh->PrimitiveIndexResource.GetGpuVirtualAddress();
				}

				Mesh.MaterialIndex = NumMaterials;
				Mesh.NumMeshlets   = StaticMesh.Mesh->MeshletCount;

				Mesh.BoundingBox = StaticMesh.Mesh->BoundingBox;

				Mesh.DrawIndexedArguments = DrawIndexedArguments;

				HlslMeshes[NumMeshes] = Mesh;

				StaticMeshes.push_back(&StaticMesh);

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

	RHI::D3D12CommandContext& Copy = Device->GetDevice()->GetCopyContext1();
	Copy.Open();
	{
		Copy.ResetCounter(&IndirectCommandBuffer, CommandBufferCounterOffset);
	}
	Copy.Close();
	RHI::D3D12SyncHandle CopySyncHandle = Copy.Execute(false);

	RHI::D3D12SyncHandle ComputeSyncHandle;
	if (NumMeshes > 0)
	{
		RHI::D3D12CommandContext& AsyncCompute = Device->GetDevice()->GetAsyncComputeCommandContext();
		AsyncCompute.GetCommandQueue()->WaitForSyncHandle(CopySyncHandle);

		AsyncCompute.Open();
		{
			D3D12ScopedEvent(AsyncCompute, "Gpu Frustum Culling");
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

	RHI::RenderGraph Graph(Allocator, Registry);

	struct GBufferParameters
	{
		RHI::RgResourceHandle Normal;
		RHI::RgResourceHandle MaterialId;
		RHI::RgResourceHandle Motion;
		RHI::RgResourceHandle Depth;

		RHI::RgResourceHandle RtvNormal;
		RHI::RgResourceHandle RtvMaterialId;
		RHI::RgResourceHandle RtvMotion;
		RHI::RgResourceHandle Dsv;

		RHI::RgResourceHandle SrvNormal;
		RHI::RgResourceHandle SrvMaterialId;
		RHI::RgResourceHandle SrvMotion;
		RHI::RgResourceHandle SrvDepth;
	} GBufferArgs;
	FLOAT Color[]	   = { 0, 0, 0, 0 };
	GBufferArgs.Normal = Graph.Create<RHI::D3D12Texture>(
		RHI::RgTextureDesc("Normal")
			.SetFormat(DXGI_FORMAT_R32G32B32A32_FLOAT)
			.SetExtent(View.Width, View.Height, 1)
			.AllowRenderTarget()
			.SetClearValue(CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R32G32B32A32_FLOAT, Color)));
	GBufferArgs.MaterialId = Graph.Create<RHI::D3D12Texture>(
		RHI::RgTextureDesc("Material Id")
			.SetFormat(DXGI_FORMAT_R32_UINT)
			.SetExtent(View.Width, View.Height, 1)
			.AllowRenderTarget()
			.SetClearValue(CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R32_UINT, Color)));
	GBufferArgs.Motion = Graph.Create<RHI::D3D12Texture>(
		RHI::RgTextureDesc("Motion")
			.SetFormat(DXGI_FORMAT_R16G16_FLOAT)
			.SetExtent(View.Width, View.Height, 1)
			.AllowRenderTarget()
			.SetClearValue(CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R16G16_FLOAT, Color)));
	GBufferArgs.Depth = Graph.Create<RHI::D3D12Texture>(
		RHI::RgTextureDesc("Depth")
			.SetFormat(DXGI_FORMAT_D32_FLOAT)
			.SetExtent(View.Width, View.Height, 1)
			.AllowDepthStencil()
			.SetClearValue(CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 1.0f, 0xFF)));

	GBufferArgs.RtvNormal	  = Graph.Create<RHI::D3D12RenderTargetView>(RHI::RgViewDesc().SetResource(GBufferArgs.Normal).AsRtv(false));
	GBufferArgs.RtvMaterialId = Graph.Create<RHI::D3D12RenderTargetView>(RHI::RgViewDesc().SetResource(GBufferArgs.MaterialId).AsRtv(false));
	GBufferArgs.RtvMotion	  = Graph.Create<RHI::D3D12RenderTargetView>(RHI::RgViewDesc().SetResource(GBufferArgs.Motion).AsRtv(false));
	GBufferArgs.Dsv			  = Graph.Create<RHI::D3D12DepthStencilView>(RHI::RgViewDesc().SetResource(GBufferArgs.Depth).AsDsv());

	GBufferArgs.SrvNormal	  = Graph.Create<RHI::D3D12ShaderResourceView>(RHI::RgViewDesc().SetResource(GBufferArgs.Normal).AsTextureSrv());
	GBufferArgs.SrvMaterialId = Graph.Create<RHI::D3D12ShaderResourceView>(RHI::RgViewDesc().SetResource(GBufferArgs.MaterialId).AsTextureSrv());
	GBufferArgs.SrvMotion	  = Graph.Create<RHI::D3D12ShaderResourceView>(RHI::RgViewDesc().SetResource(GBufferArgs.Motion).AsTextureSrv());
	GBufferArgs.SrvDepth	  = Graph.Create<RHI::D3D12ShaderResourceView>(RHI::RgViewDesc().SetResource(GBufferArgs.Depth).AsTextureSrv());

#if USE_MESH_SHADERS
	Graph.AddRenderPass("GBuffer (Mesh Shaders)")
		.Write(&GBufferArgs.Albedo)
		.Write(&GBufferArgs.Normal)
		.Write(&GBufferArgs.Motion)
		.Write(&GBufferArgs.Depth)
		.Execute([=, this](RenderGraphRegistry& Registry, D3D12CommandContext& Context)
				 {
					 Context.SetPipelineState(Registry.GetPipelineState(PipelineStates::Meshlet));
					 Context.SetGraphicsRootSignature(Registry.GetRootSignature(RootSignatures::Meshlet));
					 Context.SetGraphicsConstantBuffer(5, sizeof(GlobalConstants), &g_GlobalConstants);
					 Context->SetGraphicsRootShaderResourceView(6, Materials.GetGpuVirtualAddress());
					 Context->SetGraphicsRootShaderResourceView(7, Meshes.GetGpuVirtualAddress());

					 Context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
					 Context.SetViewport(RHIViewport(0.0f, 0.0f, View.Width, View.Height, 0.0f, 1.0f));
					 Context.SetScissorRect(RHIRect(0, 0, View.Width, View.Height));

					 RHI::D3D12RenderTargetView* RenderTargetViews[3] = {
						 Registry.Get<RHI::D3D12RenderTargetView>(GBufferArgs.AlbedoRtv),
						 Registry.Get<RHI::D3D12RenderTargetView>(GBufferArgs.NormalRtv),
						 Registry.Get<RHI::D3D12RenderTargetView>(GBufferArgs.MotionRtv)
					 };
					 RHI::D3D12DepthStencilView* DepthStencilView = Registry.Get<RHI::D3D12DepthStencilView>(GBufferArgs.Dsv);

					 Context.ClearRenderTarget(3, RenderTargetViews, DepthStencilView);
					 Context.SetRenderTarget(3, RenderTargetViews, DepthStencilView);

					 Context->ExecuteIndirect(
						 CommandSignature,
						 World::MeshLimit,
						 IndirectCommandBuffer.GetResource(),
						 0,
						 IndirectCommandBuffer.GetResource(),
						 CommandBufferCounterOffset);
				 });
#else
	Graph.AddRenderPass("GBuffer")
		.Write(&GBufferArgs.Normal)
		.Write(&GBufferArgs.MaterialId)
		.Write(&GBufferArgs.Motion)
		.Write(&GBufferArgs.Depth)
		.Execute([=, this](RHI::RenderGraphRegistry& Registry, RHI::D3D12CommandContext& Context)
				 {
					 Context.SetPipelineState(Registry.GetPipelineState(PipelineStates::GBuffer));
					 Context.SetGraphicsRootSignature(Registry.GetRootSignature(RootSignatures::GBuffer));
					 Context.SetGraphicsConstantBuffer(1, sizeof(GlobalConstants), &g_GlobalConstants);
					 Context->SetGraphicsRootShaderResourceView(2, Materials.GetGpuVirtualAddress());
					 Context->SetGraphicsRootShaderResourceView(3, Lights.GetGpuVirtualAddress());
					 Context->SetGraphicsRootShaderResourceView(4, Meshes.GetGpuVirtualAddress());

					 Context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
					 Context.SetViewport(RHIViewport(0.0f, 0.0f, View.Width, View.Height, 0.0f, 1.0f));
					 Context.SetScissorRect(RHIRect(0, 0, View.Width, View.Height));

					 RHI::D3D12RenderTargetView* RenderTargetViews[3] = {
						 Registry.Get<RHI::D3D12RenderTargetView>(GBufferArgs.RtvNormal),
						 Registry.Get<RHI::D3D12RenderTargetView>(GBufferArgs.RtvMaterialId),
						 Registry.Get<RHI::D3D12RenderTargetView>(GBufferArgs.RtvMotion)
					 };
					 RHI::D3D12DepthStencilView* DepthStencilView = Registry.Get<RHI::D3D12DepthStencilView>(GBufferArgs.Dsv);

					 Context.ClearRenderTarget(3, RenderTargetViews, DepthStencilView);
					 Context.SetRenderTarget(3, RenderTargetViews, DepthStencilView);

					 Context->ExecuteIndirect(
						 CommandSignature,
						 World::MeshLimit,
						 IndirectCommandBuffer.GetResource(),
						 0,
						 IndirectCommandBuffer.GetResource(),
						 CommandBufferCounterOffset);
				 });
#endif //

	RHI::RgResourceHandle Views[] = {
		GBufferArgs.SrvNormal,
		GBufferArgs.SrvMaterialId,
		GBufferArgs.SrvMotion,
		GBufferArgs.SrvDepth,
	};

	Graph.GetEpiloguePass()
		.Read(GBufferArgs.Normal)
		.Read(GBufferArgs.MaterialId)
		.Read(GBufferArgs.Motion)
		.Read(GBufferArgs.Depth);

	Graph.Execute(Context);
	Viewport = reinterpret_cast<void*>(Registry.Get<RHI::D3D12ShaderResourceView>(Views[ViewMode])->GetGpuHandle().ptr);
}

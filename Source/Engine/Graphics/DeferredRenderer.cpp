#include "DeferredRenderer.h"

#include "DebugRenderer.h"
#include "RendererRegistry.h"

void DeferredRenderer::SetViewportResolution(uint32_t Width, uint32_t Height)
{
	Resolution.RefreshViewportResolution(Width, Height);
	Resolution.RefreshRenderResolution(Width, Height);
}

void DeferredRenderer::Initialize()
{
	Shaders::Compile();
	// Libraries::Compile();
	RenderPasses::Compile();
	RootSignatures::Compile(RenderDevice);
	PipelineStates::Compile(RenderDevice);
	// RaytracingPipelineStates::Compile(RenderDevice);

	DEBUG_RENDERER_INITIALIZE(&RenderPasses::GBufferRenderPass);

#if USE_MESH_SHADERS
	CommandSignatureBuilder Builder(6, sizeof(CommandSignatureParams));
	Builder.AddConstant(0, 0, 1);
	Builder.AddShaderResourceView(1);
	Builder.AddShaderResourceView(2);
	Builder.AddShaderResourceView(3);
	Builder.AddShaderResourceView(4);
	Builder.AddDispatchMesh();
#else
	CommandSignatureBuilder Builder(4, sizeof(CommandSignatureParams));
	Builder.AddConstant(0, 0, 1);
	Builder.AddVertexBufferView(0);
	Builder.AddIndexBufferView();
	Builder.AddDrawIndexed();
#endif

#if USE_MESH_SHADERS
	CommandSignature = D3D12CommandSignature(
		RenderCore::Device,
		Builder,
		RenderDevice.GetRootSignature(RootSignatures::Meshlet)->GetApiHandle());
#else
	CommandSignature = D3D12CommandSignature(
		RenderCore::Device,
		Builder,
		RenderDevice.GetRootSignature(RootSignatures::GBuffer)->GetApiHandle());
#endif

	IndirectCommandBuffer = D3D12Buffer(
		RenderCore::Device->GetDevice(),
		CommandBufferCounterOffset + sizeof(UINT64),
		sizeof(CommandSignatureParams),
		D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	UAV = D3D12UnorderedAccessView(RenderCore::Device->GetDevice());
	IndirectCommandBuffer.CreateUnorderedAccessView(UAV, World::MeshLimit, CommandBufferCounterOffset);

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

	StaticMeshes.reserve(World::MeshLimit);
	HlslMeshes.resize(World::MeshLimit);
}

void DeferredRenderer::Destroy()
{
	DEBUG_RENDERER_SHUTDOWN();
}

void DeferredRenderer::Render(World* World, D3D12CommandContext& Context)
{
	if (ImGui::Begin("Renderer"))
	{
		ImGui::Text("Deferred Renderer");
		constexpr const char* View[] = { "Albedo", "Normal", "Motion", "Depth" };
		ImGui::Combo("View", &ViewMode, View, static_cast<int>(std::size(View)));

		ImGui::Checkbox("Debug Renderer", &DebugRenderer::Enable);

		ImGui::Text("Render resolution: %dx%d", Resolution.RenderWidth, Resolution.RenderHeight);
		ImGui::Text("Viewport resolution: %dx%d", Resolution.ViewportWidth, Resolution.ViewportHeight);
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

	D3D12CommandContext& Copy = RenderCore::Device->GetDevice()->GetCopyContext1();
	Copy.OpenCommandList();
	{
		Copy.ResetCounter(&IndirectCommandBuffer, CommandBufferCounterOffset);
	}
	Copy.CloseCommandList();
	D3D12SyncHandle CopySyncHandle = Copy.Execute(false);

	D3D12SyncHandle ComputeSyncHandle;
	if (NumMeshes > 0)
	{
		D3D12CommandContext& AsyncCompute = RenderCore::Device->GetDevice()->GetAsyncComputeCommandContext();
		AsyncCompute.GetCommandQueue()->WaitForSyncHandle(CopySyncHandle);

		AsyncCompute.OpenCommandList();
		{
#if USE_MESH_SHADERS
			AsyncCompute.SetPipelineState(RenderDevice.GetPipelineState(PipelineStates::IndirectCullMeshShader));
#else
			AsyncCompute.SetPipelineState(RenderDevice.GetPipelineState(PipelineStates::IndirectCull));
#endif
			AsyncCompute.SetComputeRootSignature(RenderDevice.GetRootSignature(RootSignatures::IndirectCull));

			AsyncCompute.SetComputeConstantBuffer(0, sizeof(GlobalConstants), &g_GlobalConstants);
			AsyncCompute->SetComputeRootShaderResourceView(1, Meshes.GetGpuVirtualAddress());
			AsyncCompute->SetComputeRootDescriptorTable(2, UAV.GetGpuHandle());

			AsyncCompute.Dispatch1D<128>(NumMeshes);
		}
		AsyncCompute.CloseCommandList();
		ComputeSyncHandle = AsyncCompute.Execute(false);
	}

	Context.GetCommandQueue()->WaitForSyncHandle(ComputeSyncHandle);

	RenderGraph RenderGraph(Allocator, Scheduler, Registry, Resolution);

	struct GBufferParams
	{
		RenderResourceHandle Albedo;
		RenderResourceHandle Normal;
		RenderResourceHandle Motion;
		RenderResourceHandle Depth;
		RenderResourceHandle RenderTarget;
	};

#if USE_MESH_SHADERS
	RenderPass* GBuffer = RenderGraph.AddRenderPass(
		"GBuffer (Mesh Shaders)",
		[&](RenderGraphScheduler& Scheduler, RenderScope& Scope)
		{
			FLOAT Color[] = { 0, 0, 0, 0 };

			auto&		Params	 = Scope.Get<GBufferParams>();
			const auto& ViewData = Scope.Get<RenderGraphViewData>();

			Params.Albedo = Scheduler.CreateTexture(
				"Albedo",
				TextureDesc::Texture2D(
					ETextureResolution::Render,
					DXGI_FORMAT_R32G32B32A32_FLOAT,
					1,
					TextureFlag_AllowRenderTarget,
					CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R32G32B32A32_FLOAT, Color)));
			Params.Normal = Scheduler.CreateTexture(
				"Normal",
				TextureDesc::Texture2D(
					ETextureResolution::Render,
					DXGI_FORMAT_R32G32B32A32_FLOAT,
					1,
					TextureFlag_AllowRenderTarget,
					CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R32G32B32A32_FLOAT, Color)));
			Params.Motion = Scheduler.CreateTexture(
				"Motion",
				TextureDesc::Texture2D(
					ETextureResolution::Render,
					DXGI_FORMAT_R16G16_FLOAT,
					1,
					TextureFlag_AllowRenderTarget,
					CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R16G16_FLOAT, Color)));
			Params.Depth = Scheduler.CreateTexture(
				"Depth",
				TextureDesc::Texture2D(
					ETextureResolution::Render,
					DXGI_FORMAT_D32_FLOAT,
					1,
					TextureFlag_AllowDepthStencil,
					CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 1.0f, 0xFF)));

			{
				RGRenderTargetDesc Desc = {};
				Desc.AddRenderTarget(Params.Albedo, false);
				Desc.AddRenderTarget(Params.Normal, false);
				Desc.AddRenderTarget(Params.Motion, false);
				Desc.SetDepthStencil(Params.Depth);

				Params.RenderTarget = Scheduler.CreateRenderTarget(Desc);
			}

			return [=, &Params, &ViewData, this](RenderGraphRegistry& Registry, D3D12CommandContext& Context)
			{
				D3D12ScopedEvent(Context, "GBuffer (Mesh Shaders)");

				Context.SetPipelineState(RenderDevice.GetPipelineState(PipelineStates::Meshlet));
				Context.SetGraphicsRootSignature(RenderDevice.GetRootSignature(RootSignatures::Meshlet));
				Context.SetGraphicsConstantBuffer(5, sizeof(GlobalConstants), &g_GlobalConstants);
				Context->SetGraphicsRootShaderResourceView(6, Materials.GetGpuVirtualAddress());
				Context->SetGraphicsRootShaderResourceView(7, Meshes.GetGpuVirtualAddress());

				Context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				Context.SetViewport(
					RHIViewport(0.0f, 0.0f, ViewData.GetRenderWidth<FLOAT>(), ViewData.GetRenderHeight<FLOAT>()));
				Context.SetScissorRect(RHIRect(0, 0, ViewData.RenderWidth, ViewData.RenderHeight));

				Context.BeginRenderPass(
					&RenderPasses::GBufferRenderPass,
					&Registry.GetRenderTarget(Params.RenderTarget));
				{
					/*for (auto [i, MeshRenderer] : enumerate(MeshRenderers))
					{
						auto& Mesh = MeshRenderer->pMeshFilter->Mesh;

						Context->SetGraphicsRoot32BitConstant(0, i, 0);
						Context->SetGraphicsRootShaderResourceView(1, Mesh->VertexResource.GetGpuVirtualAddress());
						Context->SetGraphicsRootShaderResourceView(2, Mesh->MeshletResource.GetGpuVirtualAddress());
						Context->SetGraphicsRootShaderResourceView(
							3,
							Mesh->UniqueVertexIndexResource.GetGpuVirtualAddress());
						Context->SetGraphicsRootShaderResourceView(
							4,
							Mesh->PrimitiveIndexResource.GetGpuVirtualAddress());

						Context.DispatchMesh(Mesh->Meshlets.size(), 1, 1);
					}*/

					Context->ExecuteIndirect(
						CommandSignature,
						World::MeshLimit,
						IndirectCommandBuffer,
						0,
						IndirectCommandBuffer,
						CommandBufferCounterOffset);

					DEBUG_RENDERER_RENDER(g_GlobalConstants.Camera.ViewProjection, Context);
				}
				Context.EndRenderPass();
			};
		});
#else
	RenderPass* GBuffer = RenderGraph.AddRenderPass(
		"GBuffer",
		[&](RenderGraphScheduler& Scheduler, RenderScope& Scope)
		{
			FLOAT Color[] = { 0, 0, 0, 0 };

			auto&		Params	 = Scope.Get<GBufferParams>();
			const auto& ViewData = Scope.Get<RenderGraphViewData>();

			Params.Albedo = Scheduler.CreateTexture(
				"Albedo",
				TextureDesc::Texture2D(
					ETextureResolution::Render,
					DXGI_FORMAT_R32G32B32A32_FLOAT,
					1,
					TextureFlag_AllowRenderTarget,
					CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R32G32B32A32_FLOAT, Color)));
			Params.Normal = Scheduler.CreateTexture(
				"Normal",
				TextureDesc::Texture2D(
					ETextureResolution::Render,
					DXGI_FORMAT_R32G32B32A32_FLOAT,
					1,
					TextureFlag_AllowRenderTarget,
					CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R32G32B32A32_FLOAT, Color)));
			Params.Motion = Scheduler.CreateTexture(
				"Motion",
				TextureDesc::Texture2D(
					ETextureResolution::Render,
					DXGI_FORMAT_R16G16_FLOAT,
					1,
					TextureFlag_AllowRenderTarget,
					CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R16G16_FLOAT, Color)));
			Params.Depth = Scheduler.CreateTexture(
				"Depth",
				TextureDesc::Texture2D(
					ETextureResolution::Render,
					DXGI_FORMAT_D32_FLOAT,
					1,
					TextureFlag_AllowDepthStencil,
					CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 1.0f, 0xFF)));

			{
				RGRenderTargetDesc Desc = {};
				Desc.AddRenderTarget(Params.Albedo, false);
				Desc.AddRenderTarget(Params.Normal, false);
				Desc.AddRenderTarget(Params.Motion, false);
				Desc.SetDepthStencil(Params.Depth);

				Params.RenderTarget = Scheduler.CreateRenderTarget(Desc);
			}

			return [=, &Params, &ViewData, this](RenderGraphRegistry& Registry, D3D12CommandContext& Context)
			{
				D3D12ScopedEvent(Context, "GBuffer");

				Context.SetPipelineState(RenderDevice.GetPipelineState(PipelineStates::GBuffer));
				Context.SetGraphicsRootSignature(RenderDevice.GetRootSignature(RootSignatures::GBuffer));
				Context.SetGraphicsConstantBuffer(1, sizeof(GlobalConstants), &g_GlobalConstants);
				Context->SetGraphicsRootShaderResourceView(2, Materials.GetGpuVirtualAddress());
				Context->SetGraphicsRootShaderResourceView(3, Lights.GetGpuVirtualAddress());
				Context->SetGraphicsRootShaderResourceView(4, Meshes.GetGpuVirtualAddress());

				Context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				Context.SetViewport(
					RHIViewport(0.0f, 0.0f, ViewData.GetRenderWidth<FLOAT>(), ViewData.GetRenderHeight<FLOAT>()));
				Context.SetScissorRect(RHIRect(0, 0, ViewData.RenderWidth, ViewData.RenderHeight));

				Context.BeginRenderPass(
					&RenderPasses::GBufferRenderPass,
					&Registry.GetRenderTarget(Params.RenderTarget));
				{
					/*for (auto [i, MeshRenderer] : enumerate(MeshRenderers))
					{
						Context->SetGraphicsRoot32BitConstant(0, i, 0);

						D3D12Buffer& VertexBuffer = MeshRenderer->pMeshFilter->Mesh->VertexResource;
						D3D12Buffer& IndexBuffer  = MeshRenderer->pMeshFilter->Mesh->IndexResource;

						D3D12_VERTEX_BUFFER_VIEW VertexBufferView = VertexBuffer.GetVertexBufferView();
						D3D12_INDEX_BUFFER_VIEW	 IndexBufferView  = IndexBuffer.GetIndexBufferView();

						Context->IASetVertexBuffers(0, 1, &VertexBufferView);
						Context->IASetIndexBuffer(&IndexBufferView);

						Context->DrawIndexedInstanced(MeshRenderer->pMeshFilter->Mesh->Indices.size(), 1, 0, 0, 0);
					}*/
					Context->ExecuteIndirect(
						CommandSignature,
						World::MeshLimit,
						IndirectCommandBuffer,
						0,
						IndirectCommandBuffer,
						CommandBufferCounterOffset);

					DEBUG_RENDERER_RENDER(g_GlobalConstants.Camera.ViewProjection, Context);
				}
				Context.EndRenderPass();
			};
		});
#endif //

	auto&				 Params	 = GBuffer->Scope.Get<GBufferParams>();
	RenderResourceHandle Views[] = {
		Params.Albedo,
		Params.Normal,
		Params.Motion,
		Params.Depth,
	};

	RenderGraph.Setup();
	RenderGraph.Compile();
	RenderGraph.Execute(Context);
	RenderGraph.RenderGui();

	ValidViewport = true;
	Viewport	  = reinterpret_cast<void*>(Registry.GetTextureSRV(Views[ViewMode]).GetGpuHandle().ptr);
}

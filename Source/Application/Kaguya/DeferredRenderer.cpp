#include "DeferredRenderer.h"
#include <imgui.h>
#include "RendererRegistry.h"

DeferredRenderer::DeferredRenderer(
	RHI::D3D12Device* Device,
	ShaderCompiler*	  Compiler)
	: Renderer(Device, Compiler)
{
	Shaders::Compile(Compiler);
	RootSignatures::Compile(Device, Registry);
	PipelineStates::Compile(Device, Registry);

#if USE_MESH_SHADERS
	RHI::CommandSignatureDesc Builder(6, sizeof(CommandSignatureParams));
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
	CommandSignature = RHI::D3D12CommandSignature(
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
		Device->GetLinkedDevice(),
		CommandBufferCounterOffset + sizeof(UINT64),
		sizeof(CommandSignatureParams),
		D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	IndirectCommandBufferUav = RHI::D3D12UnorderedAccessView(Device->GetLinkedDevice(), &IndirectCommandBuffer, World::MeshLimit, CommandBufferCounterOffset);
}

void DeferredRenderer::RenderOptions()
{
	constexpr const char* View[] = { "Normal", "Material Id", "Motion", "Depth" };
	ImGui::Combo("GBuffer View", &ViewMode, View, static_cast<int>(std::size(View)));
}

void DeferredRenderer::Render(World* World, WorldRenderView* WorldRenderView, RHI::D3D12CommandContext& Context)
{
	WorldRenderView->Update(World, nullptr);
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
	g_GlobalConstants.Camera	= GetHLSLCameraDesc(*WorldRenderView->Camera);
	g_GlobalConstants.NumMeshes = WorldRenderView->NumMeshes;
	g_GlobalConstants.NumLights = WorldRenderView->NumLights;

	RHI::D3D12SyncHandle ComputeSyncHandle;
	if (WorldRenderView->NumMeshes > 0)
	{
		RHI::D3D12CommandContext& Copy = Device->GetLinkedDevice()->GetCopyContext1();
		Copy.Open();
		{
			Copy.ResetCounter(&IndirectCommandBuffer, CommandBufferCounterOffset);
		}
		Copy.Close();
		RHI::D3D12SyncHandle CopySyncHandle = Copy.Execute(false);

		RHI::D3D12CommandContext& AsyncCompute = Device->GetLinkedDevice()->GetAsyncComputeCommandContext();
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
			AsyncCompute->SetComputeRootShaderResourceView(1, WorldRenderView->Meshes.GetGpuVirtualAddress());
			AsyncCompute->SetComputeRootDescriptorTable(2, IndirectCommandBufferUav.GetGpuHandle());

			AsyncCompute.Dispatch1D<128>(WorldRenderView->NumMeshes);
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
	constexpr float Color[] = { 0, 0, 0, 0 };
	GBufferArgs.Normal		= Graph.Create<RHI::D3D12Texture>(
		 RHI::RgTextureDesc("Normal")
			 .SetFormat(DXGI_FORMAT_R32G32B32A32_FLOAT)
			 .SetExtent(WorldRenderView->View.Width, WorldRenderView->View.Height, 1)
			 .SetAllowRenderTarget()
			 .SetClearValue(RHI::RgClearValue(Color)));
	GBufferArgs.MaterialId = Graph.Create<RHI::D3D12Texture>(
		RHI::RgTextureDesc("Material Id")
			.SetFormat(DXGI_FORMAT_R32_UINT)
			.SetExtent(WorldRenderView->View.Width, WorldRenderView->View.Height, 1)
			.SetAllowRenderTarget()
			.SetClearValue(RHI::RgClearValue(Color)));
	GBufferArgs.Motion = Graph.Create<RHI::D3D12Texture>(
		RHI::RgTextureDesc("Motion")
			.SetFormat(DXGI_FORMAT_R16G16_FLOAT)
			.SetExtent(WorldRenderView->View.Width, WorldRenderView->View.Height, 1)
			.SetAllowRenderTarget()
			.SetClearValue(RHI::RgClearValue(Color)));
	GBufferArgs.Depth = Graph.Create<RHI::D3D12Texture>(
		RHI::RgTextureDesc("Depth")
			.SetFormat(DXGI_FORMAT_D32_FLOAT)
			.SetExtent(WorldRenderView->View.Width, WorldRenderView->View.Height, 1)
			.SetAllowDepthStencil()
			.SetClearValue(RHI::RgClearValue(1.0f, 0xff)));

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
		.Write(&GBufferArgs.Normal)
		.Write(&GBufferArgs.MaterialId)
		.Write(&GBufferArgs.Motion)
		.Write(&GBufferArgs.Depth)
		.Execute([=, this](RHI::RenderGraphRegistry& Registry, RHI::D3D12CommandContext& Context)
				 {
					 Context.SetPipelineState(Registry.GetPipelineState(PipelineStates::Meshlet));
					 Context.SetGraphicsRootSignature(Registry.GetRootSignature(RootSignatures::Meshlet));
					 Context.SetGraphicsConstantBuffer(5, sizeof(GlobalConstants), &g_GlobalConstants);
					 Context->SetGraphicsRootShaderResourceView(6, WorldRenderView->Materials.GetGpuVirtualAddress());
					 Context->SetGraphicsRootShaderResourceView(7, WorldRenderView->Meshes.GetGpuVirtualAddress());

					 Context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
					 Context.SetViewport(RHIViewport(0.0f, 0.0f, View.Width, View.Height, 0.0f, 1.0f));
					 Context.SetScissorRect(RHIRect(0, 0, View.Width, View.Height));

					 RHI::D3D12RenderTargetView* RenderTargetViews[3] = {
						 Registry.Get<RHI::D3D12RenderTargetView>(GBufferArgs.RtvNormal),
						 Registry.Get<RHI::D3D12RenderTargetView>(GBufferArgs.RtvMaterialId),
						 Registry.Get<RHI::D3D12RenderTargetView>(GBufferArgs.RtvMotion)
					 };
					 RHI::D3D12DepthStencilView* DepthStencilView = Registry.Get<RHI::D3D12DepthStencilView>(GBufferArgs.Dsv);

					 Context.ClearRenderTarget(RenderTargetViews, DepthStencilView);
					 Context.SetRenderTarget(RenderTargetViews, DepthStencilView);

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
		.Write({ &GBufferArgs.Normal, &GBufferArgs.MaterialId, &GBufferArgs.Motion, &GBufferArgs.Depth })
		.Execute([=, this](RHI::RenderGraphRegistry& Registry, RHI::D3D12CommandContext& Context)
				 {
					 Context.SetPipelineState(Registry.GetPipelineState(PipelineStates::GBuffer));
					 Context.SetGraphicsRootSignature(Registry.GetRootSignature(RootSignatures::GBuffer));
					 Context.SetGraphicsConstantBuffer(1, sizeof(GlobalConstants), &g_GlobalConstants);
					 Context->SetGraphicsRootShaderResourceView(2, WorldRenderView->Materials.GetGpuVirtualAddress());
					 Context->SetGraphicsRootShaderResourceView(3, WorldRenderView->Lights.GetGpuVirtualAddress());
					 Context->SetGraphicsRootShaderResourceView(4, WorldRenderView->Meshes.GetGpuVirtualAddress());

					 Context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
					 Context.SetViewport(RHIViewport(0.0f, 0.0f, static_cast<float>(WorldRenderView->View.Width), static_cast<float>(WorldRenderView->View.Height), 0.0f, 1.0f));
					 Context.SetScissorRect(RHIRect(0, 0, WorldRenderView->View.Width, WorldRenderView->View.Height));

					 RHI::D3D12RenderTargetView* RenderTargetViews[3] = {
						 Registry.Get<RHI::D3D12RenderTargetView>(GBufferArgs.RtvNormal),
						 Registry.Get<RHI::D3D12RenderTargetView>(GBufferArgs.RtvMaterialId),
						 Registry.Get<RHI::D3D12RenderTargetView>(GBufferArgs.RtvMotion)
					 };
					 RHI::D3D12DepthStencilView* DepthStencilView = Registry.Get<RHI::D3D12DepthStencilView>(GBufferArgs.Dsv);

					 Context.ClearRenderTarget(RenderTargetViews, DepthStencilView);
					 Context.SetRenderTarget(RenderTargetViews, DepthStencilView);

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
		.Read({ GBufferArgs.Normal, GBufferArgs.MaterialId, GBufferArgs.Motion, GBufferArgs.Depth });

	Graph.Execute(Context);
	Viewport = reinterpret_cast<void*>(Registry.Get<RHI::D3D12ShaderResourceView>(Views[ViewMode])->GetGpuHandle().ptr);
}

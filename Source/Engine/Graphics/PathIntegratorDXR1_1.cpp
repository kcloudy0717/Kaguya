#include "PathIntegratorDXR1_1.h"
#include "RendererRegistry.h"

void PathIntegratorDXR1_1::SetViewportResolution(uint32_t Width, uint32_t Height)
{
	Resolution.RefreshViewportResolution(Width, Height);
	Resolution.RefreshRenderResolution(Width, Height);
}

void PathIntegratorDXR1_1::Initialize()
{
	Shaders::Compile();
	Libraries::Compile();
	RenderPasses::Compile();
	RootSignatures::Compile(RenderDevice);
	PipelineStates::Compile(RenderDevice);

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

	D3D12SyncHandle ASBuildSyncHandle;
	if (AccelerationStructure.IsValid())
	{
		D3D12CommandContext& AsyncCompute = RenderCore::Device->GetDevice()->GetAsyncComputeCommandContext();
		AsyncCompute.OpenCommandList();
		{
			D3D12ScopedEvent(AsyncCompute, "TLAS");
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

	RenderGraph RenderGraph(Allocator, Scheduler, Registry, Resolution);

	struct PathTraceParameter
	{
		RenderResourceHandle Output;
	};
	RenderPass* PathTrace = RenderGraph.AddRenderPass(
		"Path Trace",
		[&](RenderGraphScheduler& Scheduler, RenderScope& Scope)
		{
			auto&		Parameter = Scope.Get<PathTraceParameter>();
			const auto& ViewData  = Scope.Get<RenderGraphViewData>();

			Parameter.Output = Scheduler.CreateTexture(
				"Path Trace Output",
				TextureDesc::RWTexture2D(ETextureResolution::Render, DXGI_FORMAT_R32G32B32A32_FLOAT, 1));

			return [=, &Parameter, &ViewData, this](RenderGraphRegistry& Registry, D3D12CommandContext& Context)
			{
				D3D12ScopedEvent(Context, "Path Trace");

				if (!AccelerationStructure.IsValid())
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
					unsigned int	 AntiAliasing;
				} g_GlobalConstants						= {};
				g_GlobalConstants.Camera				= GetHLSLCameraDesc(*World->ActiveCamera);
				g_GlobalConstants.Resolution			= { static_cast<float>(Resolution.ViewportWidth),
													static_cast<float>(Resolution.ViewportHeight),
													1.0f / static_cast<float>(Resolution.ViewportWidth),
													1.0f / static_cast<float>(Resolution.ViewportHeight) };
				g_GlobalConstants.NumLights				= NumLights;
				g_GlobalConstants.TotalFrameCount		= FrameCounter++;
				g_GlobalConstants.MaxDepth				= PathIntegratorState.MaxDepth;
				g_GlobalConstants.NumAccumulatedSamples = NumTemporalSamples++;
				g_GlobalConstants.RenderTarget			= Registry.GetRWTextureIndex(Parameter.Output);
				g_GlobalConstants.SkyIntensity			= PathIntegratorState.SkyIntensity;
				g_GlobalConstants.Dimensions			= { ViewData.RenderWidth, ViewData.RenderHeight };
				g_GlobalConstants.AntiAliasing			= PathIntegratorState.Antialiasing;

				Context.SetPipelineState(RenderDevice.GetPipelineState(PipelineStates::RTX::PathTrace));
				Context.SetComputeRootSignature(RenderDevice.GetRootSignature(RootSignatures::RTX::PathTrace));
				Context.SetComputeConstantBuffer(0, sizeof(GlobalConstants), &g_GlobalConstants);
				Context->SetComputeRootShaderResourceView(1, AccelerationStructure);
				Context->SetComputeRootShaderResourceView(2, Materials.GetGpuVirtualAddress());
				Context->SetComputeRootShaderResourceView(3, Lights.GetGpuVirtualAddress());

				Context.Dispatch2D<16, 16>(ViewData.RenderWidth, ViewData.RenderHeight);
				Context.UAVBarrier(nullptr);
			};
		});

	struct TonemapParameter
	{
		RenderResourceHandle Output;
		RenderResourceHandle RenderTarget;
	};
	RenderPass* Tonemap = RenderGraph.AddRenderPass(
		"Tonemap",
		[&](RenderGraphScheduler& Scheduler, RenderScope& Scope)
		{
			FLOAT Color[]	 = { 1, 1, 1, 1 };
			auto  ClearValue = CD3DX12_CLEAR_VALUE(D3D12SwapChain::Format_sRGB, Color);

			auto&		Parameter = Scope.Get<TonemapParameter>();
			const auto& ViewData  = Scope.Get<RenderGraphViewData>();

			Parameter.Output = Scheduler.CreateTexture(
				"Tonemap Output",
				TextureDesc::Texture2D(
					ETextureResolution::Render,
					D3D12SwapChain::Format,
					1,
					TextureFlag_AllowRenderTarget,
					ClearValue));

			{
				RGRenderTargetDesc Desc = {};
				Desc.AddRenderTarget(Parameter.Output, true);

				Parameter.RenderTarget = Scheduler.CreateRenderTarget(Desc);
			}

			auto PathTraceInput = Scheduler.Read(PathTrace->Scope.Get<PathTraceParameter>().Output);

			return [=, &Parameter, &ViewData, this](RenderGraphRegistry& Registry, D3D12CommandContext& Context)
			{
				struct RootConstants
				{
					unsigned int InputIndex;
				} Constants;
				Constants.InputIndex = Registry.GetTextureIndex(PathTraceInput);

				D3D12ScopedEvent(Context, "Tonemap");

				Context.SetPipelineState(RenderDevice.GetPipelineState(PipelineStates::Tonemap));
				Context.SetGraphicsRootSignature(RenderDevice.GetRootSignature(RootSignatures::Tonemap));
				Context->SetGraphicsRoot32BitConstants(0, 1, &Constants, 0);

				Context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				Context.SetViewport(
					RHIViewport(0.0f, 0.0f, ViewData.GetRenderWidth<FLOAT>(), ViewData.GetRenderHeight<FLOAT>()));
				Context.SetScissorRect(RHIRect(0, 0, ViewData.RenderWidth, ViewData.RenderHeight));

				Context.BeginRenderPass(
					&RenderPasses::TonemapRenderPass,
					&Registry.GetRenderTarget(Parameter.RenderTarget));
				{
					Context.DrawInstanced(3, 1, 0, 0);
				}
				Context.EndRenderPass();
			};
		});

	RenderGraph.Setup();
	RenderGraph.Compile();
	RenderGraph.Execute(Context);
	RenderGraph.RenderGui();

	RenderResourceHandle Handle = Tonemap->Scope.Get<TonemapParameter>().Output;
	ValidViewport				= true;
	Viewport					= reinterpret_cast<void*>(Registry.GetTextureSRV(Handle).GetGpuHandle().ptr);
}

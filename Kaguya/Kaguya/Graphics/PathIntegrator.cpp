#include "PathIntegrator.h"
#include "RendererRegistry.h"

// CAS
#define A_CPU
#include <Graphics/FidelityFX-FSR/ffx_a.h>
#include <Graphics/FidelityFX-FSR/ffx_fsr1.h>

_declspec(align(256)) struct FSRConstants
{
	DirectX::XMUINT4 Const0;
	DirectX::XMUINT4 Const1;
	DirectX::XMUINT4 Const2;
	DirectX::XMUINT4 Const3;
	DirectX::XMUINT4 Sample;
};

void* PathIntegrator::GetViewportDescriptor()
{
	if (ValidViewport)
	{
		return reinterpret_cast<void*>(Registry.GetTextureSRV(Viewport).GetGpuHandle().ptr);
	}

	return nullptr;
}

void PathIntegrator::SetViewportResolution(uint32_t Width, uint32_t Height)
{
	if (Resolution.ViewportWidth != Width || Resolution.ViewportHeight != Height)
	{
		Resolution.ViewportWidth			 = Width;
		Resolution.ViewportHeight			 = Height;
		Resolution.ViewportResolutionResized = true;
	}

	ValidViewport = false;

	float r = 1.5f;
	switch (FSRState.QualityMode)
	{
	case EFSRQualityMode::Ultra:
		r = 1.3f;
		break;
	case EFSRQualityMode::Standard:
		r = 1.5f;
		break;
	case EFSRQualityMode::Balanced:
		r = 1.7f;
		break;
	case EFSRQualityMode::Performance:
		r = 2.0f;
		break;
	}

	UINT RenderWidth, RenderHeight;
	if (FSRState.Enable)
	{
		RenderWidth	 = static_cast<UINT>(Resolution.ViewportWidth / r);
		RenderHeight = static_cast<UINT>(Resolution.ViewportHeight / r);
	}
	else
	{
		RenderWidth	 = Resolution.ViewportWidth;
		RenderHeight = Resolution.ViewportHeight;
	}

	if (Resolution.RenderWidth != RenderWidth || Resolution.RenderHeight != RenderHeight)
	{
		Resolution.RenderWidth			   = RenderWidth;
		Resolution.RenderHeight			   = RenderHeight;
		Resolution.RenderResolutionResized = true;
	}
}

void PathIntegrator::Initialize()
{
	Shaders::Compile();
	Libraries::Compile();
	RootSignatures::Compile(RenderDevice);
	PipelineStates::Compile(RenderDevice);
	RaytracingPipelineStates::Compile(RenderDevice);

	{
		RenderPassDesc Desc		= {};
		FLOAT		   Color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		Desc.AddRenderTarget({ .Format	   = D3D12SwapChain::RHIFormat,
							   .LoadOp	   = ELoadOp::Clear,
							   .StoreOp	   = EStoreOp::Store,
							   .ClearValue = ClearValue(D3D12SwapChain::RHIFormat, Color) });
		TonemapRenderPass = D3D12RenderPass(RenderCore::Device, Desc);
	}

	AccelerationStructure = RaytracingAccelerationStructure(1, World::InstanceLimit);
	AccelerationStructure.Initialize();

	Manager = D3D12RaytracingAccelerationStructureManager(RenderCore::Device->GetDevice(), 6_MiB);

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

	RayGenerationShaderTable = ShaderBindingTable.AddRayGenerationShaderTable<void>(1);
	RayGenerationShaderTable->AddShaderRecord(RaytracingPipelineStates::g_RayGenerationSID);

	MissShaderTable = ShaderBindingTable.AddMissShaderTable<void>(2);
	MissShaderTable->AddShaderRecord(RaytracingPipelineStates::g_MissSID);
	MissShaderTable->AddShaderRecord(RaytracingPipelineStates::g_ShadowMissSID);

	HitGroupShaderTable = ShaderBindingTable.AddHitGroupShaderTable<RootArgument>(World::InstanceLimit);

	ShaderBindingTable.Generate(RenderCore::Device->GetDevice());
}

void PathIntegrator::Render(World* World, D3D12CommandContext& Context)
{
	bool ResetPathIntegrator = false;

	if (ImGui::Begin("Path Integrator"))
	{
		if (ImGui::Button("Restore Defaults"))
		{
			PathIntegratorState = {};
			ResetPathIntegrator = true;
		}

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

	FSRState.RenderWidth	= Resolution.RenderWidth;
	FSRState.RenderHeight	= Resolution.RenderHeight;
	FSRState.ViewportWidth	= Resolution.ViewportWidth;
	FSRState.ViewportHeight = Resolution.ViewportHeight;
	if (ImGui::Begin("FSR"))
	{
		bool Dirty = ImGui::Checkbox("Enable", &FSRState.Enable);

		constexpr const char* QualityModes[] = { "Ultra Quality (1.3x)",
												 "Quality (1.5x)",
												 "Balanced (1.7x)",
												 "Performance (2x)" };
		Dirty |= ImGui::Combo(
			"Quality",
			reinterpret_cast<int*>(&FSRState.QualityMode),
			QualityModes,
			static_cast<int>(std::size(QualityModes)));

		Dirty |= ImGui::SliderFloat("Sharpening attenuation", &FSRState.RCASAttenuation, 0.0f, 2.0f);

		ImGui::Text("Render resolution: %dx%d", Resolution.RenderWidth, Resolution.RenderHeight);
		ImGui::Text("Viewport resolution: %dx%d", Resolution.ViewportWidth, Resolution.ViewportHeight);

		ResetPathIntegrator |= Dirty;
	}
	ImGui::End();

	NumMaterials = NumLights = 0;
	AccelerationStructure.Reset();
	World->Registry.view<Transform, MeshFilter, MeshRenderer>().each(
		[&](auto&& Transform, auto&& MeshFilter, auto&& MeshRenderer)
		{
			if (MeshFilter.Mesh)
			{
				AccelerationStructure.AddInstance(Transform, &MeshRenderer);
				pMaterial[NumMaterials++] = GetHLSLMaterialDesc(MeshRenderer.Material);
			}
		});
	World->Registry.view<Transform, Light>().each(
		[&](auto&& Transform, auto&& Light)
		{
			pLights[NumLights++] = GetHLSLLightDesc(Transform, Light);
		});

	D3D12CommandSyncPoint CopySyncPoint;
	if (AccelerationStructure.IsValid())
	{
		D3D12CommandContext& Copy = RenderCore::Device->GetDevice()->GetCopyContext1();
		Copy.OpenCommandList();

		// Update shader table
		HitGroupShaderTable->Reset();
		for (auto [i, MeshRenderer] : enumerate(AccelerationStructure.MeshRenderers))
		{
			ID3D12Resource* VertexBuffer = MeshRenderer->pMeshFilter->Mesh->VertexResource.GetResource();
			ID3D12Resource* IndexBuffer	 = MeshRenderer->pMeshFilter->Mesh->IndexResource.GetResource();

			D3D12RaytracingShaderTable<RootArgument>::Record Record = {};
			Record.ShaderIdentifier									= RaytracingPipelineStates::g_DefaultSID;
			Record.RootArguments.MaterialIndex						= static_cast<UINT>(i);
			Record.RootArguments.Padding							= 0xDEADBEEF;
			Record.RootArguments.VertexBuffer						= VertexBuffer->GetGPUVirtualAddress();
			Record.RootArguments.IndexBuffer						= IndexBuffer->GetGPUVirtualAddress();

			HitGroupShaderTable->AddShaderRecord(Record);
		}

		ShaderBindingTable.Write();
		ShaderBindingTable.CopyToGpu(Copy);

		Copy.CloseCommandList();

		CopySyncPoint = Copy.Execute(false);
	}

	D3D12CommandSyncPoint ASBuildSyncPoint;
	if (AccelerationStructure.IsValid())
	{
		D3D12CommandContext& AsyncCompute = RenderCore::Device->GetDevice()->GetAsyncComputeCommandContext();
		AsyncCompute.OpenCommandList();
		{
			bool AnyBuild = false;

			for (auto Geometry : AccelerationStructure.ReferencedGeometries)
			{
				if (Geometry->BlasValid)
				{
					continue;
				}

				Geometry->BlasValid = true;
				AnyBuild |= Geometry->BlasValid;

				D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS Inputs = Geometry->Blas.GetInputsDesc();
				Geometry->BlasIndex = Manager.Build(AsyncCompute.CommandListHandle.GetGraphicsCommandList4(), Inputs);
			}

			if (AnyBuild)
			{
				AsyncCompute.UAVBarrier(nullptr);
				AsyncCompute.FlushResourceBarriers();
				Manager.Copy(AsyncCompute.CommandListHandle.GetGraphicsCommandList4());
			}

			{
				D3D12ScopedEvent(AsyncCompute, "BLAS Compact");
				for (auto Geometry : AccelerationStructure.ReferencedGeometries)
				{
					Manager.Compact(AsyncCompute.CommandListHandle.GetGraphicsCommandList4(), Geometry->BlasIndex);
				}
			}

			for (auto Geometry : AccelerationStructure.ReferencedGeometries)
			{
				Geometry->AccelerationStructure = Manager.GetAccelerationStructureAddress(Geometry->BlasIndex);
			}

			D3D12ScopedEvent(AsyncCompute, "TLAS");
			AccelerationStructure.Build(AsyncCompute);
		}
		AsyncCompute.CloseCommandList();

		ASBuildSyncPoint = AsyncCompute.Execute(false);

		for (auto Geometry : AccelerationStructure.ReferencedGeometries)
		{
			if (Geometry->BlasIndex != UINT64_MAX)
			{
				Manager.SetSyncPoint(Geometry->BlasIndex, ASBuildSyncPoint);
			}
		}
	}

	if (World->WorldState & EWorldState::EWorldState_Update)
	{
		World->WorldState	= EWorldState_Render;
		ResetPathIntegrator = true;
	}

	Context.GetCommandQueue()->WaitForSyncPoint(CopySyncPoint);
	Context.GetCommandQueue()->WaitForSyncPoint(ASBuildSyncPoint);

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
				RGTextureDesc::RWTexture2D(ETextureResolution::Render, DXGI_FORMAT_R32G32B32A32_FLOAT, 1));

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

				Context.SetPipelineState(RenderDevice.GetRaytracingPipelineState(RaytracingPipelineStates::RTPSO));
				Context.SetComputeRootSignature(RenderDevice.GetRootSignature(RaytracingPipelineStates::GlobalRS));
				Context.SetComputeConstantBuffer(0, sizeof(GlobalConstants), &g_GlobalConstants);
				Context->SetComputeRootShaderResourceView(1, AccelerationStructure);
				Context->SetComputeRootShaderResourceView(2, Materials.GetGpuVirtualAddress());
				Context->SetComputeRootShaderResourceView(3, Lights.GetGpuVirtualAddress());

				D3D12_DISPATCH_RAYS_DESC Desc = ShaderBindingTable.GetDesc(0, 0);
				Desc.Width					  = ViewData.RenderWidth;
				Desc.Height					  = ViewData.RenderHeight;
				Desc.Depth					  = 1;

				Context.DispatchRays(&Desc);
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
				RGTextureDesc::Texture2D(
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

				Context.BeginRenderPass(&TonemapRenderPass, &Registry.GetRenderTarget(Parameter.RenderTarget));
				{
					Context.DrawInstanced(3, 1, 0, 0);
				}
				Context.EndRenderPass();
			};
		});

	struct FSRParameter
	{
		RenderResourceHandle EASUOutput;
		RenderResourceHandle RCASOutput;
	};
	RenderPass* FSR = RenderGraph.AddRenderPass(
		"FSR",
		[&](RenderGraphScheduler& Scheduler, RenderScope& Scope)
		{
			auto&		Parameter = Scope.Get<FSRParameter>();
			const auto& ViewData  = Scope.Get<RenderGraphViewData>();

			Parameter.EASUOutput = Scheduler.CreateTexture(
				"EASU Output",
				RGTextureDesc::RWTexture2D(ETextureResolution::Viewport, D3D12SwapChain::Format, 1));
			Parameter.RCASOutput = Scheduler.CreateTexture(
				"RCAS Output",
				RGTextureDesc::RWTexture2D(ETextureResolution::Viewport, D3D12SwapChain::Format, 1));

			auto TonemapInput = Scheduler.Read(Tonemap->Scope.Get<TonemapParameter>().Output);
			return [=, &Parameter, &ViewData, this](RenderGraphRegistry& Registry, D3D12CommandContext& Context)
			{
				struct RootConstants
				{
					unsigned int InputTID;
					unsigned int OutputTID;
				};

				D3D12ScopedEvent(Context, "FSR");

				Context.SetComputeRootSignature(RenderDevice.GetRootSignature(RootSignatures::FSR));

				Context.TransitionBarrier(
					&Registry.GetTexture(Parameter.EASUOutput),
					D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

				{
					D3D12ScopedEvent(Context, "EASU");

					Context.SetPipelineState(RenderDevice.GetPipelineState(PipelineStates::FSREASU));

					RootConstants RC	  = { Registry.GetTextureIndex(TonemapInput),
										  Registry.GetRWTextureIndex(Parameter.EASUOutput) };
					FSRConstants  FsrEasu = {};
					FsrEasuCon(
						reinterpret_cast<AU1*>(&FsrEasu.Const0),
						reinterpret_cast<AU1*>(&FsrEasu.Const1),
						reinterpret_cast<AU1*>(&FsrEasu.Const2),
						reinterpret_cast<AU1*>(&FsrEasu.Const3),
						static_cast<AF1>(FSRState.RenderWidth),
						static_cast<AF1>(FSRState.RenderHeight),
						static_cast<AF1>(FSRState.RenderWidth),
						static_cast<AF1>(FSRState.RenderHeight),
						static_cast<AF1>(ViewData.ViewportWidth),
						static_cast<AF1>(ViewData.ViewportHeight));
					FsrEasu.Sample.x = 0;

					Context->SetComputeRoot32BitConstants(0, 2, &RC, 0);
					Context.SetComputeConstantBuffer(1, sizeof(FSRConstants), &FsrEasu);

					Context.Dispatch2D<16, 16>(ViewData.ViewportWidth, ViewData.ViewportHeight);
				}

				Context.TransitionBarrier(
					&Registry.GetTexture(Parameter.EASUOutput),
					D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				Context.TransitionBarrier(
					&Registry.GetTexture(Parameter.RCASOutput),
					D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

				{
					D3D12ScopedEvent(Context, "RCAS");

					Context.SetPipelineState(RenderDevice.GetPipelineState(PipelineStates::FSRRCAS));

					RootConstants RC	  = { Registry.GetTextureIndex(Parameter.EASUOutput),
										  Registry.GetRWTextureIndex(Parameter.RCASOutput) };
					FSRConstants  FsrRcas = {};
					FsrRcasCon(reinterpret_cast<AU1*>(&FsrRcas.Const0), FSRState.RCASAttenuation);
					FsrRcas.Sample.x = 0;

					Context->SetComputeRoot32BitConstants(0, 2, &RC, 0);
					Context.SetComputeConstantBuffer(1, sizeof(FSRConstants), &FsrRcas);

					Context.Dispatch2D<16, 16>(ViewData.ViewportWidth, ViewData.ViewportHeight);
				}

				Context.TransitionBarrier(
					&Registry.GetTexture(Parameter.RCASOutput),
					D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
				Context.FlushResourceBarriers();
			};
		});

	if (FSRState.Enable)
	{
		Viewport = FSR->Scope.Get<FSRParameter>().RCASOutput;
	}
	else
	{
		Viewport = Tonemap->Scope.Get<TonemapParameter>().Output;
	}
	ValidViewport = true;

	RenderGraph.Setup();
	RenderGraph.Compile();
	RenderGraph.Execute(Context);

	if (ImGui::Begin("Render Graph"))
	{
		for (const auto& RenderPass : RenderGraph)
		{
			char Label[MAX_PATH] = {};
			sprintf_s(Label, "Pass: %s", RenderPass->Name.data());
			if (ImGui::TreeNode(Label))
			{
				constexpr ImGuiTableFlags TableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable |
													   ImGuiTableFlags_Hideable | ImGuiTableFlags_RowBg |
													   ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV;

				ImGui::Text("Inputs");
				if (ImGui::BeginTable("Inputs", 1, TableFlags))
				{
					for (auto Handle : RenderPass->Reads)
					{
						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0);

						ImGui::Text("%s", RenderGraph.GetScheduler().GetTextureName(Handle).data());
					}
					ImGui::EndTable();
				}

				ImGui::Text("Outputs");
				if (ImGui::BeginTable("Outputs", 1, TableFlags))
				{
					for (auto Handle : RenderPass->Writes)
					{
						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0);

						ImGui::Text("%s", RenderGraph.GetScheduler().GetTextureName(Handle).data());
					}
					ImGui::EndTable();
				}

				ImGui::TreePop();
			}
		}
	}
	ImGui::End();
}

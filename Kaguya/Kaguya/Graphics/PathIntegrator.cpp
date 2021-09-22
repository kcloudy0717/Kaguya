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

struct PathTrace
{
	RenderResourceHandle Output;
};

struct Tonemap
{
	RenderResourceHandle Output;
	RenderResourceHandle RenderTarget;
};

struct FSR
{
	RenderResourceHandle EASUOutput;
	RenderResourceHandle RCASOutput;
};

void* PathIntegrator::GetViewportDescriptor()
{
	if (RenderGraph.ValidViewport)
	{
		RenderResourceHandle TonemapOutput = RenderGraph.GetScope("Tonemap").Get<Tonemap>().Output;
		RenderResourceHandle FSROutput	   = RenderGraph.GetScope("FSR").Get<FSR>().RCASOutput;

		return FSRState.Enable
				   ? reinterpret_cast<void*>(RenderGraph.GetRegistry().GetTextureSRV(FSROutput).GetGpuHandle().ptr)
				   : reinterpret_cast<void*>(RenderGraph.GetRegistry().GetTextureSRV(TonemapOutput).GetGpuHandle().ptr);
	}

	return nullptr;
}

void PathIntegrator::SetViewportResolution(uint32_t Width, uint32_t Height)
{
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

	if (FSRState.Enable)
	{
		RenderWidth	 = static_cast<UINT>(ViewportWidth / r);
		RenderHeight = static_cast<UINT>(ViewportHeight / r);
	}
	else
	{
		RenderWidth	 = ViewportWidth;
		RenderHeight = ViewportHeight;
	}
}

void PathIntegrator::Initialize()
{
	Shaders::Compile(*RenderCore::pShaderCompiler);
	Libraries::Compile(*RenderCore::pShaderCompiler);
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
		TonemapRenderPass = D3D12RenderPass(RenderCore::pDevice, Desc);
	}

	AccelerationStructure = RaytracingAccelerationStructure(1);
	AccelerationStructure.Initialize();

	Manager = D3D12RaytracingAccelerationStructureManager(RenderCore::pDevice->GetDevice(), 6_MiB);

	Materials = D3D12Buffer(
		RenderCore::pDevice->GetDevice(),
		sizeof(HLSL::Material) * World::MaterialLimit,
		sizeof(HLSL::Material),
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_RESOURCE_FLAG_NONE);
	Materials.Initialize();
	pMaterial = Materials.GetCpuVirtualAddress<HLSL::Material>();

	Lights = D3D12Buffer(
		RenderCore::pDevice->GetDevice(),
		sizeof(HLSL::Light) * World::LightLimit,
		sizeof(HLSL::Light),
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_RESOURCE_FLAG_NONE);
	Lights.Initialize();
	pLights = Lights.GetCpuVirtualAddress<HLSL::Light>();

	RenderGraph.AddRenderPass(
		"Path Trace",
		[&](RenderGraphScheduler& Scheduler, RenderScope& Scope)
		{
			auto&		Parameter = Scope.Get<PathTrace>();
			const auto& ViewData  = Scope.Get<RenderGraphViewData>();

			Parameter.Output = Scheduler.CreateTexture(
				"Path Trace Output",
				RGTextureDesc::RWTexture2D(ETextureResolution::Render, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, 1));

			return [&Parameter, &ViewData, this](RenderGraphRegistry& Registry, D3D12CommandContext& Context)
			{
				D3D12ScopedEvent(Context, "Path Trace");

				if (!AccelerationStructure.Valid())
				{
					return;
				}

				static unsigned int Counter = 0;
				_declspec(align(256)) struct GlobalConstants
				{
					HLSL::Camera Camera;

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
				g_GlobalConstants.Camera				= GetHLSLCameraDesc(*pWorld->ActiveCamera);
				g_GlobalConstants.Resolution			= { static_cast<float>(ViewportWidth),
													static_cast<float>(ViewportHeight),
													1.0f / static_cast<float>(ViewportWidth),
													1.0f / static_cast<float>(ViewportHeight) };
				g_GlobalConstants.NumLights				= NumLights;
				g_GlobalConstants.TotalFrameCount		= Counter++;
				g_GlobalConstants.MaxDepth				= PathIntegratorState.MaxDepth;
				g_GlobalConstants.NumAccumulatedSamples = Settings::NumAccumulatedSamples++;
				g_GlobalConstants.RenderTarget			= Registry.GetRWTextureIndex(Parameter.Output);
				g_GlobalConstants.SkyIntensity			= PathIntegratorState.SkyIntensity;

				D3D12Allocation Allocation = Context.CpuConstantAllocator.Allocate(g_GlobalConstants);

				Context.SetPipelineState(RenderDevice.GetRaytracingPipelineState(RaytracingPipelineStates::RTPSO));
				Context->SetComputeRootSignature(*RenderDevice.GetRootSignature(RaytracingPipelineStates::GlobalRS));
				Context->SetComputeRootConstantBufferView(0, Allocation.GpuVirtualAddress);
				Context->SetComputeRootShaderResourceView(1, AccelerationStructure);
				Context->SetComputeRootShaderResourceView(2, Materials.GetGpuVirtualAddress());
				Context->SetComputeRootShaderResourceView(3, Lights.GetGpuVirtualAddress());

				RenderCore::pDevice->BindComputeDescriptorTable(
					*RenderDevice.GetRootSignature(RaytracingPipelineStates::GlobalRS),
					Context);

				D3D12_DISPATCH_RAYS_DESC Desc = ShaderBindingTable.GetDesc(0, 0);
				Desc.Width					  = ViewData.RenderWidth;
				Desc.Height					  = ViewData.RenderHeight;
				Desc.Depth					  = 1;

				Context.DispatchRays(&Desc);
				Context.UAVBarrier(nullptr);
			};
		});

	RenderGraph.AddRenderPass(
		"Tonemap",
		[&](RenderGraphScheduler& Scheduler, RenderScope& Scope)
		{
			auto& PathTraceData = Scheduler.GetParentRenderGraph()->GetScope("Path Trace").Get<PathTrace>();

			FLOAT Color[]	 = { 1, 1, 1, 1 };
			auto  ClearValue = CD3DX12_CLEAR_VALUE(D3D12SwapChain::Format_sRGB, Color);

			auto&		Parameter = Scope.Get<Tonemap>();
			const auto& ViewData  = Scope.Get<RenderGraphViewData>();

			Parameter.Output = Scheduler.CreateTexture(
				"Tonemap Output",
				RGTextureDesc::Texture2D(
					ETextureResolution::Render,
					D3D12SwapChain::Format,
					0,
					0,
					1,
					TextureFlag_AllowRenderTarget,
					ClearValue));

			{
				RGRenderTargetDesc Desc = {};
				Desc.AddRenderTarget(Parameter.Output, true);

				Parameter.RenderTarget = Scheduler.CreateRenderTarget(Desc);
			}

			auto PathTraceInput = Scheduler.Read(PathTraceData.Output);
			return [=, &Parameter, &ViewData, this](RenderGraphRegistry& Registry, D3D12CommandContext& Context)
			{
				D3D12ScopedEvent(Context, "Tonemap");

				Context.SetPipelineState(RenderDevice.GetPipelineState(PipelineStates::Tonemap));
				Context->SetGraphicsRootSignature(*RenderDevice.GetRootSignature(RootSignatures::Tonemap));

				Context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				Context.SetViewport(
					RHIViewport(0.0f, 0.0f, ViewData.GetRenderWidth<FLOAT>(), ViewData.GetRenderHeight<FLOAT>()));
				Context.SetScissorRect(RHIRect(0, 0, ViewData.RenderWidth, ViewData.RenderHeight));

				Context.BeginRenderPass(&TonemapRenderPass, &Registry.GetRenderTarget(Parameter.RenderTarget));
				{
					struct RootConstants
					{
						unsigned int InputIndex;
					} Constants;
					Constants.InputIndex = Registry.GetTextureIndex(PathTraceInput);

					Context->SetGraphicsRoot32BitConstants(0, 1, &Constants, 0);

					Context.DrawInstanced(3, 1, 0, 0);
				}
				Context.EndRenderPass();
			};
		});

	RenderGraph.AddRenderPass(
		"FSR",
		[&](RenderGraphScheduler& Scheduler, RenderScope& Scope)
		{
			auto& TonemapScope = Scheduler.GetParentRenderGraph()->GetScope("Tonemap");

			auto&		Parameter = Scope.Get<FSR>();
			const auto& ViewData  = Scope.Get<RenderGraphViewData>();

			Parameter.EASUOutput = Scheduler.CreateTexture(
				"EASU Output",
				RGTextureDesc::RWTexture2D(ETextureResolution::Viewport, D3D12SwapChain::Format, 0, 0, 1));
			Parameter.RCASOutput = Scheduler.CreateTexture(
				"RCAS Output",
				RGTextureDesc::RWTexture2D(ETextureResolution::Viewport, D3D12SwapChain::Format, 0, 0, 1));

			auto TonemapInput = Scheduler.Read(TonemapScope.Get<Tonemap>().Output);
			return [=, &Parameter, &ViewData, this](RenderGraphRegistry& Registry, D3D12CommandContext& Context)
			{
				struct RootConstants
				{
					unsigned int InputTID;
					unsigned int OutputTID;
				};

				D3D12ScopedEvent(Context, "FSR");

				Context->SetComputeRootSignature(*RenderDevice.GetRootSignature(RootSignatures::FSR));

				// This value is the image region dimension that each thread group of the FSR shader operates on
				constexpr UINT ThreadSize		 = 16;
				UINT		   ThreadGroupCountX = (ViewData.ViewportWidth + (ThreadSize - 1)) / ThreadSize;
				UINT		   ThreadGroupCountY = (ViewData.ViewportHeight + (ThreadSize - 1)) / ThreadSize;

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

					D3D12Allocation Allocation = Context.CpuConstantAllocator.Allocate(sizeof(FSRConstants));
					std::memcpy(Allocation.CpuVirtualAddress, &FsrEasu, sizeof(FSRConstants));

					Context->SetComputeRoot32BitConstants(0, 2, &RC, 0);
					Context->SetComputeRootConstantBufferView(1, Allocation.GpuVirtualAddress);

					Context.Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
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

					D3D12Allocation Allocation = Context.CpuConstantAllocator.Allocate(sizeof(FSRConstants));
					std::memcpy(Allocation.CpuVirtualAddress, &FsrRcas, sizeof(FSRConstants));

					Context->SetComputeRoot32BitConstants(0, 2, &RC, 0);
					Context->SetComputeRootConstantBufferView(1, Allocation.GpuVirtualAddress);

					Context.Dispatch(ThreadGroupCountX, ThreadGroupCountY, 1);
				}

				Context.TransitionBarrier(
					&Registry.GetTexture(Parameter.RCASOutput),
					D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
				Context.FlushResourceBarriers();
			};
		});

	RenderGraph.Setup();
	RenderGraph.Compile();

	RayGenerationShaderTable = ShaderBindingTable.AddRayGenerationShaderTable<void>(1);
	RayGenerationShaderTable->AddShaderRecord(RaytracingPipelineStates::g_RayGenerationSID);

	MissShaderTable = ShaderBindingTable.AddMissShaderTable<void>(2);
	MissShaderTable->AddShaderRecord(RaytracingPipelineStates::g_MissSID);
	MissShaderTable->AddShaderRecord(RaytracingPipelineStates::g_ShadowMissSID);

	HitGroupShaderTable = ShaderBindingTable.AddHitGroupShaderTable<RootArgument>(World::InstanceLimit);

	ShaderBindingTable.Generate(RenderCore::pDevice->GetDevice());
}

void PathIntegrator::Render(D3D12CommandContext& Context)
{
	if (ImGui::Begin("Path Integrator"))
	{
		bool Dirty = false;

		if (ImGui::Button("Restore Defaults"))
		{
			PathIntegratorState = {};
			Dirty = true;
		}

		Dirty |= ImGui::SliderFloat("Sky Intensity", &PathIntegratorState.SkyIntensity, 0.0f, 5.0f);
		Dirty |= ImGui::SliderScalar(
			"Max Depth",
			ImGuiDataType_U32,
			&PathIntegratorState.MaxDepth,
			&PathIntegratorState.MinimumDepth,
			&PathIntegratorState.MaximumDepth);

		if (Dirty)
		{
			Settings::NumAccumulatedSamples = 0;
		}
		ImGui::Text("Num Samples Accumulated: %u", Settings::NumAccumulatedSamples);
	}
	ImGui::End();

	FSRState.RenderWidth	= RenderWidth;
	FSRState.RenderHeight	= RenderHeight;
	FSRState.ViewportWidth	= ViewportWidth;
	FSRState.ViewportHeight = ViewportHeight;
	if (ImGui::Begin("FSR"))
	{
		ImGui::Checkbox("Enable", &FSRState.Enable);

		constexpr const char* QualityModes[] = { "Ultra Quality (1.3x)",
												 "Quality (1.5x)",
												 "Balanced (1.7x)",
												 "Performance (2x)" };
		ImGui::Combo(
			"Quality",
			reinterpret_cast<int*>(&FSRState.QualityMode),
			QualityModes,
			static_cast<int>(std::size(QualityModes)));

		ImGui::SliderFloat("Sharpening attenuation", &FSRState.RCASAttenuation, 0.0f, 2.0f);

		ImGui::Text("Render resolution: %dx%d", RenderWidth, RenderHeight);
		ImGui::Text("Viewport resolution: %dx%d", ViewportWidth, ViewportHeight);
	}
	ImGui::End();

	NumMaterials = NumLights = 0;
	AccelerationStructure.Reset();
	pWorld->Registry.view<Transform, MeshFilter, MeshRenderer>().each(
		[&](auto&& Transform, auto&& MeshFilter, auto&& MeshRenderer)
		{
			if (MeshFilter.Mesh)
			{
				AccelerationStructure.AddInstance(Transform, &MeshRenderer);
				pMaterial[NumMaterials++] = GetHLSLMaterialDesc(MeshRenderer.Material);
			}
		});
	pWorld->Registry.view<Transform, Light>().each(
		[&](auto&& Transform, auto&& Light)
		{
			pLights[NumLights++] = GetHLSLLightDesc(Transform, Light);
		});

	D3D12CommandSyncPoint CopySyncPoint;
	if (AccelerationStructure.Valid())
	{
		D3D12CommandContext& Copy = RenderCore::pDevice->GetDevice()->GetCopyContext1();
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
	if (AccelerationStructure.Valid())
	{
		D3D12CommandContext& AsyncCompute = RenderCore::pDevice->GetDevice()->GetAsyncComputeCommandContext();
		AsyncCompute.OpenCommandList();

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

		for (auto Geometry : AccelerationStructure.ReferencedGeometries)
		{
			Manager.Compact(AsyncCompute.CommandListHandle.GetGraphicsCommandList4(), Geometry->BlasIndex);
		}

		for (auto Geometry : AccelerationStructure.ReferencedGeometries)
		{
			Geometry->AccelerationStructure = Manager.GetAccelerationStructureAddress(Geometry->BlasIndex);
		}

		AccelerationStructure.Build(AsyncCompute);

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

	if (pWorld->WorldState & EWorldState::EWorldState_Update)
	{
		pWorld->WorldState				= EWorldState_Render;
		Settings::NumAccumulatedSamples = 0;
	}

	Context.GetCommandQueue()->WaitForSyncPoint(CopySyncPoint);
	Context.GetCommandQueue()->WaitForSyncPoint(ASBuildSyncPoint);

	RenderGraph.Execute(Context);
	RenderGraph.ValidViewport = true;
}

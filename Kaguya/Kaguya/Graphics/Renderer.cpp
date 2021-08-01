#include "Renderer.h"

#include "World/Entity.h"

#include <wincodec.h> // GUID for different file formats, needed for ScreenGrab

#include "RendererRegistry.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

// CAS
#define A_CPU
#include <Graphics/FidelityFX-FSR/ffx_a.h>
#include <Graphics/FidelityFX-FSR/ffx_fsr1.h>

struct RootConstants
{
	unsigned int InputTID;
	unsigned int OutputTID;
};

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

	RenderTargetView RTV;
};

struct FSR
{
	RenderResourceHandle EASUOutput;
	RenderResourceHandle RCASOutput;
};

void Renderer::SetViewportResolution(uint32_t Width, uint32_t Height)
{
	if (Width == 0 || Height == 0)
	{
		return;
	}

	ViewportWidth  = Width;
	ViewportHeight = Height;

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
		RenderWidth	 = UINT(ViewportWidth / r);
		RenderHeight = UINT(ViewportHeight / r);
	}
	else
	{
		RenderWidth	 = ViewportWidth;
		RenderHeight = ViewportHeight;
	}

	RenderGraph.SetResolution(RenderWidth, RenderHeight, ViewportWidth, ViewportHeight);
	RenderGraph.GetRegistry().ScheduleResources();
	RenderGraph.ValidViewport = false;
}

void* Renderer::GetViewportDescriptor()
{
	if (RenderGraph.ValidViewport)
	{
		RenderResourceHandle TonemapOutput = RenderGraph.GetScope("Tonemap").Get<Tonemap>().Output;
		RenderResourceHandle FSROutput	   = RenderGraph.GetScope("FSR").Get<FSR>().RCASOutput;

		return FSRState.Enable ? (void*)RenderGraph.GetRegistry().GetTextureSRV(FSROutput).GetGPUHandle().ptr
							   : (void*)RenderGraph.GetRegistry().GetTextureSRV(TonemapOutput).GetGPUHandle().ptr;
	}

	return nullptr;
}

void Renderer::OnInitialize()
{
	Shaders::Compile(*RenderCore::pShaderCompiler);
	Libraries::Compile(*RenderCore::pShaderCompiler);
	PipelineStates::Compile();
	RaytracingPipelineStates::Compile();

	AccelerationStructure = RaytracingAccelerationStructure(1);
	AccelerationStructure.Initialize();

	Manager = RaytracingAccelerationStructureManager(RenderCore::pAdapter->GetDevice(), 6_MiB);

	Materials = Buffer(
		RenderCore::pAdapter->GetDevice(),
		sizeof(HLSL::Material) * World::MaterialLimit,
		sizeof(HLSL::Material),
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_RESOURCE_FLAG_NONE);
	Materials.Initialize();
	pMaterials = Materials.GetCPUVirtualAddress<HLSL::Material>();

	Lights = Buffer(
		RenderCore::pAdapter->GetDevice(),
		sizeof(HLSL::Light) * World::LightLimit,
		sizeof(HLSL::Light),
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_RESOURCE_FLAG_NONE);
	Lights.Initialize();
	pLights = Lights.GetCPUVirtualAddress<HLSL::Light>();

	RenderGraph.AddRenderPass(
		"Path Trace",
		[&](RenderGraphScheduler& Scheduler, RenderScope& Scope)
		{
			auto&		Parameter = Scope.Get<PathTrace>();
			const auto& ViewData  = Scope.Get<RenderGraphViewData>();

			Parameter.Output = Scheduler.CreateTexture(
				ETextureResolution::Render,
				RGTextureDesc::RWTexture2D(DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, 1));

			return [&Parameter, &ViewData, this](RenderGraphRegistry& Registry, CommandContext& Context)
			{
				D3D12ScopedEvent(Context, "Path Trace");

				if (AccelerationStructure.empty())
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

					DirectX::XMFLOAT2 MousePosition;

					unsigned int NumLights;
					unsigned int TotalFrameCount;

					unsigned int MaxDepth;
					unsigned int NumAccumulatedSamples;

					unsigned int RenderTarget;

					float SkyIntensity;
				} g_GlobalConstants						= {};
				g_GlobalConstants.Camera				= GetHLSLCameraDesc(*World.ActiveCamera);
				g_GlobalConstants.Resolution			= { float(ViewportWidth),
													float(ViewportHeight),
													1.0f / float(ViewportWidth),
													1.0f / float(ViewportHeight) };
				g_GlobalConstants.MousePosition			= { 0.0f, 0.0f };
				g_GlobalConstants.NumLights				= NumLights;
				g_GlobalConstants.TotalFrameCount		= Counter++;
				g_GlobalConstants.MaxDepth				= PathIntegratorState.MaxDepth;
				g_GlobalConstants.NumAccumulatedSamples = Settings::NumAccumulatedSamples++;
				g_GlobalConstants.RenderTarget			= Registry.GetRWTextureIndex(Parameter.Output);
				g_GlobalConstants.SkyIntensity			= PathIntegratorState.SkyIntensity;

				Allocation Allocation = Context.CpuConstantAllocator.Allocate(g_GlobalConstants);

				Context.CommandListHandle.GetGraphicsCommandList4()->SetPipelineState1(RaytracingPipelineStates::RTPSO);
				Context->SetComputeRootSignature(RaytracingPipelineStates::GlobalRS);
				Context->SetComputeRootConstantBufferView(0, Allocation.GPUVirtualAddress);
				Context->SetComputeRootShaderResourceView(1, AccelerationStructure);
				Context->SetComputeRootShaderResourceView(2, Materials.GetGPUVirtualAddress());
				Context->SetComputeRootShaderResourceView(3, Lights.GetGPUVirtualAddress());

				RenderCore::pAdapter->BindComputeDescriptorTable(RaytracingPipelineStates::GlobalRS, Context);

				D3D12_DISPATCH_RAYS_DESC Desc = ShaderBindingTable.GetDispatchRaysDesc(0, 0);
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
			auto  ClearValue = CD3DX12_CLEAR_VALUE(SwapChain::Format_sRGB, Color);

			auto&		Parameter = Scope.Get<Tonemap>();
			const auto& ViewData  = Scope.Get<RenderGraphViewData>();

			Parameter.Output = Scheduler.CreateTexture(
				ETextureResolution::Render,
				RGTextureDesc::Texture2D(SwapChain::Format, 0, 0, 1, TextureFlag_AllowRenderTarget, ClearValue));
			Parameter.RTV = RenderTargetView(RenderCore::pAdapter->GetDevice());

			auto PathTraceInput = Scheduler.Read(PathTraceData.Output);
			return [=, &Parameter, &ViewData](RenderGraphRegistry& Registry, CommandContext& Context)
			{
				D3D12ScopedEvent(Context, "Tonemap");

				Registry.GetTexture(Parameter.Output)
					.CreateRenderTargetView(Parameter.RTV, std::nullopt, std::nullopt, std::nullopt, true);

				Context->SetPipelineState(PipelineStates::PSOs[ShaderPipelineStateId::Tonemap]);
				Context->SetGraphicsRootSignature(PipelineStates::RSs[RootSignatureId::Tonemap]);

				D3D12_VIEWPORT Viewport = CD3DX12_VIEWPORT(
					0.0f,
					0.0f,
					static_cast<FLOAT>(ViewData.RenderWidth),
					static_cast<FLOAT>(ViewData.RenderHeight));
				D3D12_RECT ScissorRect = CD3DX12_RECT(0, 0, ViewData.RenderWidth, ViewData.RenderHeight);

				Context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				Context->RSSetViewports(1, &Viewport);
				Context->RSSetScissorRects(1, &ScissorRect);
				Context->OMSetRenderTargets(1, &Parameter.RTV.GetCPUHandle(), TRUE, nullptr);

				FLOAT white[] = { 1, 1, 1, 1 };
				Context->ClearRenderTargetView(Parameter.RTV.GetCPUHandle(), white, 0, nullptr);

				struct Settings
				{
					unsigned int InputIndex;
				} settings			= {};
				settings.InputIndex = Registry.GetTextureIndex(PathTraceInput);

				Context->SetGraphicsRoot32BitConstants(0, 1, &settings, 0);

				Context.DrawInstanced(3, 1, 0, 0);
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
				ETextureResolution::Viewport,
				RGTextureDesc::RWTexture2D(SwapChain::Format, 0, 0, 1));
			Parameter.RCASOutput = Scheduler.CreateTexture(
				ETextureResolution::Viewport,
				RGTextureDesc::RWTexture2D(SwapChain::Format, 0, 0, 1));

			auto TonemapInput = Scheduler.Read(TonemapScope.Get<Tonemap>().Output);
			return [=, &Parameter, &ViewData, this](RenderGraphRegistry& Registry, CommandContext& Context)
			{
				D3D12ScopedEvent(Context, "FSR");

				Context->SetComputeRootSignature(PipelineStates::RSs[RootSignatureId::FSR]);

				// This value is the image region dimension that each thread group of the FSR shader operates on
				constexpr UINT ThreadSize		 = 16;
				UINT		   ThreadGroupCountX = (ViewData.ViewportWidth + (ThreadSize - 1)) / ThreadSize;
				UINT		   ThreadGroupCountY = (ViewData.ViewportHeight + (ThreadSize - 1)) / ThreadSize;

				Context.TransitionBarrier(
					&Registry.GetTexture(Parameter.EASUOutput),
					D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

				{
					D3D12ScopedEvent(Context, "EASU");

					Context->SetPipelineState(PipelineStates::PSOs[ShaderPipelineStateId::FSREASU]);

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
						(AF1)ViewData.ViewportWidth,
						(AF1)ViewData.ViewportHeight);
					FsrEasu.Sample.x = 0;

					Allocation Allocation = Context.CpuConstantAllocator.Allocate(sizeof(FSRConstants));
					std::memcpy(Allocation.CPUVirtualAddress, &FsrEasu, sizeof(FSRConstants));

					Context->SetComputeRoot32BitConstants(0, 2, &RC, 0);
					Context->SetComputeRootConstantBufferView(1, Allocation.GPUVirtualAddress);

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

					Context->SetPipelineState(PipelineStates::PSOs[ShaderPipelineStateId::FSRRCAS]);

					RootConstants RC	  = { Registry.GetTextureIndex(Parameter.EASUOutput),
										  Registry.GetRWTextureIndex(Parameter.RCASOutput) };
					FSRConstants  FsrRcas = {};
					FsrRcasCon(reinterpret_cast<AU1*>(&FsrRcas.Const0), FSRState.RCASAttenuation);
					FsrRcas.Sample.x = 0;

					Allocation Allocation = Context.CpuConstantAllocator.Allocate(sizeof(FSRConstants));
					std::memcpy(Allocation.CPUVirtualAddress, &FsrRcas, sizeof(FSRConstants));

					Context->SetComputeRoot32BitConstants(0, 2, &RC, 0);
					Context->SetComputeRootConstantBufferView(1, Allocation.GPUVirtualAddress);

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

	ShaderBindingTable.Generate(RenderCore::pAdapter->GetDevice());
}

void Renderer::OnRender()
{
	RenderCore::pAdapter->OnBeginFrame();

	if (ImGui::Begin("GPU Timing"))
	{
		for (const auto& iter : Profiler::Data)
		{
			for (INT i = 0; i < iter.Depth; ++i)
			{
				ImGui::Text("    ");
				ImGui::SameLine();
			}
			ImGui::Text("%s: %.2fms (%.2fms max)", iter.Name, iter.AvgTime, iter.MaxTime);
			ImGui::SameLine();
			ImGui::NewLine();
		}
	}
	ImGui::End();

	if (ImGui::Begin("Path Integrator"))
	{
		if (ImGui::Button("Restore Defaults"))
		{
			PathIntegratorState = {};
		}

		bool Dirty = false;
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

		const char* QualityModes[] = { "Ultra Quality (1.3x)",
									   "Quality (1.5x)",
									   "Balanced (1.7x)",
									   "Performance (2x)" };
		ImGui::Combo("Quality", (int*)&FSRState.QualityMode, QualityModes, static_cast<int>(std::size(QualityModes)));

		ImGui::SliderFloat("Sharpening attenuation", &FSRState.RCASAttenuation, 0.0f, 2.0f);

		ImGui::Text("Render resolution: %dx%d", RenderWidth, RenderHeight);
		ImGui::Text("Viewport resolution: %dx%d", ViewportWidth, ViewportHeight);
	}
	ImGui::End();

	World.ActiveCamera->AspectRatio = float(RenderWidth) / float(RenderHeight);

	NumMaterials = NumLights = 0;
	AccelerationStructure.clear();
	World.Registry.view<Transform, MeshFilter, MeshRenderer>().each(
		[&](auto&& Transform, auto&& MeshFilter, auto&& MeshRenderer)
		{
			if (MeshFilter.Mesh)
			{
				AccelerationStructure.AddInstance(Transform, &MeshRenderer);
				pMaterials[NumMaterials++] = GetHLSLMaterialDesc(MeshRenderer.Material);
			}
		});
	World.Registry.view<Transform, Light>().each(
		[&](auto&& Transform, auto&& Light)
		{
			pLights[NumLights++] = GetHLSLLightDesc(Transform, Light);
		});

	CommandSyncPoint CopySyncPoint;
	if (!AccelerationStructure.empty())
	{
		CommandContext& Copy = RenderCore::pAdapter->GetDevice()->GetCopyContext1();
		Copy.OpenCommandList();

		// Update shader table
		HitGroupShaderTable->Reset();
		for (auto [i, MeshRenderer] : enumerate(AccelerationStructure.MeshRenderers))
		{
			ID3D12Resource* VertexBuffer = MeshRenderer->pMeshFilter->Mesh->VertexResource.GetResource();
			ID3D12Resource* IndexBuffer	 = MeshRenderer->pMeshFilter->Mesh->IndexResource.GetResource();

			RaytracingShaderTable<RootArgument>::Record Record = {};
			Record.ShaderIdentifier							   = RaytracingPipelineStates::g_DefaultSID;
			Record.RootArguments							   = { .MaterialIndex = static_cast<UINT>(i),
									   .Padding		  = 0xDEADBEEF,
									   .VertexBuffer  = VertexBuffer->GetGPUVirtualAddress(),
									   .IndexBuffer	  = IndexBuffer->GetGPUVirtualAddress() };

			HitGroupShaderTable->AddShaderRecord(Record);
		}

		ShaderBindingTable.Write();
		ShaderBindingTable.CopyToGPU(Copy);

		Copy.CloseCommandList();

		CopySyncPoint = Copy.Execute(false);
	}

	CommandSyncPoint ASBuildSyncPoint;
	if (!AccelerationStructure.empty())
	{
		CommandContext& AsyncCompute = RenderCore::pAdapter->GetDevice()->GetAsyncComputeCommandContext();
		AsyncCompute.OpenCommandList();

		bool AnyBuild = false;

		for (auto Geometry : AccelerationStructure.ReferencedGeometries)
		{
			if (Geometry->BLASValid)
			{
				continue;
			}

			Geometry->BLASValid = true;
			AnyBuild |= Geometry->BLASValid;

			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS Inputs = Geometry->BLAS.GetInputsDesc();
			Geometry->BLASIndex = Manager.Build(AsyncCompute.CommandListHandle.GetGraphicsCommandList4(), Inputs);
		}

		if (AnyBuild)
		{
			AsyncCompute.UAVBarrier(nullptr);
			AsyncCompute.FlushResourceBarriers();
			Manager.Copy(AsyncCompute.CommandListHandle.GetGraphicsCommandList4());
		}

		for (auto Geometry : AccelerationStructure.ReferencedGeometries)
		{
			Manager.Compact(AsyncCompute.CommandListHandle.GetGraphicsCommandList4(), Geometry->BLASIndex);
		}

		for (auto Geometry : AccelerationStructure.ReferencedGeometries)
		{
			Geometry->AccelerationStructure = Manager.GetAccelerationStructureGPUVA(Geometry->BLASIndex);
		}

		AccelerationStructure.Build(AsyncCompute);

		AsyncCompute.CloseCommandList();

		ASBuildSyncPoint = AsyncCompute.Execute(false);

		for (auto Geometry : AccelerationStructure.ReferencedGeometries)
		{
			if (Geometry->BLASIndex != UINT64_MAX)
			{
				Manager.SetSyncPoint(Geometry->BLASIndex, ASBuildSyncPoint);
			}
		}
	}

	if (World.WorldState & EWorldState::EWorldState_Update)
	{
		World.WorldState				= EWorldState_Render;
		Settings::NumAccumulatedSamples = 0;
	}

	CommandContext& Context = RenderCore::pAdapter->GetDevice()->GetCommandContext();
	Context.GetCommandQueue()->WaitForSyncPoint(CopySyncPoint);
	Context.GetCommandQueue()->WaitForSyncPoint(ASBuildSyncPoint);
	Context.OpenCommandList();
	Context.BindResourceViewHeaps();

	RenderGraph.Execute(Context);
	RenderGraph.ValidViewport = true;

	auto [pRenderTarget, RenderTargetView] = RenderCore::pSwapChain->GetCurrentBackBufferResource();

	auto ResourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		pRenderTarget,
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET);
	Context->ResourceBarrier(1, &ResourceBarrier);
	{
		Viewport	= CD3DX12_VIEWPORT(0.0f, 0.0f, float(Application::GetWidth()), float(Application::GetHeight()));
		ScissorRect = CD3DX12_RECT(0, 0, Application::GetWidth(), Application::GetHeight());

		Context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		Context->RSSetViewports(1, &Viewport);
		Context->RSSetScissorRects(1, &ScissorRect);
		Context->OMSetRenderTargets(1, &RenderTargetView, TRUE, nullptr);
		FLOAT white[] = { 1, 1, 1, 1 };
		Context->ClearRenderTargetView(RenderTargetView, white, 0, nullptr);

		// ImGui Render
		{
			D3D12ScopedEvent(Context, "ImGui Render");

			ImGui::Render();
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), Context.CommandListHandle.GetGraphicsCommandList());
		}
	}
	ResourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		pRenderTarget,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT);
	Context->ResourceBarrier(1, &ResourceBarrier);

	Context.CloseCommandList();
	CommandSyncPoint MainSyncPoint = Context.Execute(false);

	RenderCore::pSwapChain->Present(false);

	RenderCore::pAdapter->OnEndFrame();

	MainSyncPoint.WaitForCompletion();
}

void Renderer::OnResize(uint32_t Width, uint32_t Height)
{
	RenderCore::pAdapter->GetDevice()->GetGraphicsQueue()->Flush();
	RenderCore::pSwapChain->Resize(Width, Height);
}

void Renderer::OnDestroy()
{
	PipelineStates::Destroy();
	RaytracingPipelineStates::Destroy();
}

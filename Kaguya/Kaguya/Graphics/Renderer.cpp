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

// Pad local root arguments explicitly
struct RootArgument
{
	UINT64					  MaterialIndex : 32;
	UINT64					  Padding		: 32;
	D3D12_GPU_VIRTUAL_ADDRESS VertexBuffer;
	D3D12_GPU_VIRTUAL_ADDRESS IndexBuffer;
};

struct PathTrace
{
	RenderResourceHandle Output;

	RaytracingShaderBindingTable		 ShaderBindingTable;
	RaytracingShaderTable<void>*		 RayGenerationShaderTable;
	RaytracingShaderTable<void>*		 MissShaderTable;
	RaytracingShaderTable<RootArgument>* HitGroupShaderTable;
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

void Renderer::SetViewportMousePosition(float MouseX, float MouseY)
{
	ViewportMouseX = MouseX;
	ViewportMouseY = MouseY;
}

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

	// PathIntegrator.SetResolution(RenderWidth, RenderHeight);
	// ToneMapper.SetResolution(RenderWidth, RenderHeight);
	// FSRFilter.SetResolution(ViewportWidth, ViewportHeight);
}

const ShaderResourceView& Renderer::GetViewportDescriptor()
{
	RenderResourceHandle TonemapOutput = RenderGraph.GetScope("Tonemap").Get<Tonemap>().Output;
	RenderResourceHandle FSROutput	   = RenderGraph.GetScope("FSR").Get<FSR>().RCASOutput;

	return FSRState.Enable ? RenderGraph.GetRegistry().GetTextureSRV(FSROutput)
						   : RenderGraph.GetRegistry().GetTextureSRV(TonemapOutput);
}

void Renderer::OnInitialize()
{
	Shaders::Compile(*RenderCore::pShaderCompiler);
	Libraries::Compile(*RenderCore::pShaderCompiler);
	PipelineStates::Compile();
	RaytracingPipelineStates::Compile();

	AccelerationStructure = RaytracingAccelerationStructure(PathIntegrator_DXR_1_0::NumHitGroups);
	AccelerationStructure.Initialize();

	Manager = RaytracingAccelerationStructureManager(RenderCore::pAdapter->GetDevice(), 6_MiB);

	// PathIntegrator.Initialize();
	// ToneMapper.Initialize();
	// FSRFilter.Initialize();

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

			Parameter.Output = Scheduler.CreateTexture(RGTextureSize::Render, RGTextureDesc());

			Parameter.RayGenerationShaderTable = Parameter.ShaderBindingTable.AddRayGenerationShaderTable<void>(1);
			Parameter.RayGenerationShaderTable->AddShaderRecord(RaytracingPipelineStates::g_RayGenerationSID);

			Parameter.MissShaderTable = Parameter.ShaderBindingTable.AddMissShaderTable<void>(2);
			Parameter.MissShaderTable->AddShaderRecord(RaytracingPipelineStates::g_MissSID);
			Parameter.MissShaderTable->AddShaderRecord(RaytracingPipelineStates::g_ShadowMissSID);

			Parameter.HitGroupShaderTable =
				Parameter.ShaderBindingTable.AddHitGroupShaderTable<RootArgument>(World::InstanceLimit);

			Parameter.ShaderBindingTable.Generate(RenderCore::pAdapter->GetDevice());

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
				g_GlobalConstants.MousePosition			= { ViewportMouseX, ViewportMouseY };
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

				D3D12_DISPATCH_RAYS_DESC Desc = Parameter.ShaderBindingTable.GetDispatchRaysDesc(0, 0);
				Desc.Width					  = ViewData.Width;
				Desc.Height					  = ViewData.Height;
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

			auto& Parameter	 = Scope.Get<Tonemap>();
			Parameter.Output = Scheduler.CreateTexture(RGTextureSize::Render, RGTextureDesc());
			Parameter.RTV	 = RenderTargetView(RenderCore::pAdapter->GetDevice());

			const auto& ViewData = Scope.Get<RenderGraphViewData>();

			auto PathTraceInput = Scheduler.Read(PathTraceData.Output);
			return [=, &Parameter, &ViewData](RenderGraphRegistry& Registry, CommandContext& Context)
			{
				D3D12ScopedEvent(Context, "Tonemap");

				Context.TransitionBarrier(&Registry.GetTexture(Parameter.Output), D3D12_RESOURCE_STATE_RENDER_TARGET);

				Context->SetPipelineState(PipelineStates::PSOs[ShaderPipelineStateId::Tonemap]);
				Context->SetGraphicsRootSignature(PipelineStates::RSs[RootSignatureId::Tonemap]);

				Context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				Context->RSSetViewports(ViewData.NumViewports, ViewData.Viewports);
				Context->RSSetScissorRects(ViewData.NumScissorRects, ViewData.ScissorRects);
				Context->OMSetRenderTargets(1, &Parameter.RTV.GetCPUHandle(), TRUE, nullptr);

				FLOAT white[] = { 1, 1, 1, 1 };
				Context->ClearRenderTargetView(Parameter.RTV.GetCPUHandle(), white, 0, nullptr);

				struct Settings
				{
					unsigned int InputIndex;
				} settings			= {};
				settings.InputIndex = Registry.GetTextureIndex(PathTraceInput);

				Context->SetGraphicsRoot32BitConstants(0, 1, &settings, 0);
				// RenderDevice::Instance().BindGraphicsDescriptorTable(m_RS, CommandList);

				Context.DrawInstanced(3, 1, 0, 0);

				Context.TransitionBarrier(
					&Registry.GetTexture(Parameter.Output),
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				Context.FlushResourceBarriers();
			};
		});

	RenderGraph.AddRenderPass(
		"FSR",
		[&](RenderGraphScheduler& Scheduler, RenderScope& Scope)
		{
			auto& TonemapScope = Scheduler.GetParentRenderGraph()->GetScope("Tonemap");

			auto& Parameter		 = Scope.Get<FSR>();
			Parameter.EASUOutput = Scheduler.CreateTexture(RGTextureSize::Viewport, RGTextureDesc());
			Parameter.RCASOutput = Scheduler.CreateTexture(RGTextureSize::Viewport, RGTextureDesc());

			const auto& ViewData = Scope.Get<RenderGraphViewData>();

			auto TonemapInput = Scheduler.Read(TonemapScope.Get<Tonemap>().Output);
			return [=, &Parameter, &ViewData, this](RenderGraphRegistry& Registry, CommandContext& Context)
			{
				D3D12ScopedEvent(Context, "FSR");

				Context->SetComputeRootSignature(PipelineStates::RSs[RootSignatureId::FSR]);

				// This value is the image region dimension that each thread group of the FSR shader operates on
				constexpr UINT ThreadSize		 = 16;
				UINT		   ThreadGroupCountX = (ViewData.Width + (ThreadSize - 1)) / ThreadSize;
				UINT		   ThreadGroupCountY = (ViewData.Height + (ThreadSize - 1)) / ThreadSize;

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
						(AF1)ViewData.Width,
						(AF1)ViewData.Height);
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
			PathIntegrator_DXR_1_0::Settings::NumAccumulatedSamples = 0;
		}
		ImGui::Text("Num Samples Accumulated: %u", PathIntegrator_DXR_1_0::Settings::NumAccumulatedSamples);
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

	World.ActiveCamera->AspectRatio = float(ViewportWidth) / float(ViewportHeight);

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
		// PathIntegrator.UpdateShaderTable(AccelerationStructure, Copy);

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
		World.WorldState = EWorldState_Render;
		// PathIntegrator.Reset();
	}

	CommandContext& Context = RenderCore::pAdapter->GetDevice()->GetCommandContext();
	Context.GetCommandQueue()->WaitForSyncPoint(CopySyncPoint);
	Context.GetCommandQueue()->WaitForSyncPoint(ASBuildSyncPoint);
	Context.OpenCommandList();

	Context.BindResourceViewHeaps();

	RenderGraph.Execute(Context);

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
}

#include "Renderer.h"
#include "Core/Stopwatch.h"

#include "RenderDevice.h"
#include "World/Entity.h"

#include <wincodec.h> // GUID for different file formats, needed for ScreenGrab

#include "RendererRegistry.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

Renderer::Renderer()
	: ViewportMouseX(0)
	, ViewportMouseY(0)
	, ViewportWidth(0)
	, ViewportHeight(0)
	, Viewport()
	, ScissorRect()
{
}

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

	PathIntegrator.SetResolution(RenderWidth, RenderHeight);
	ToneMapper.SetResolution(RenderWidth, RenderHeight);
	FSRFilter.SetResolution(ViewportWidth, ViewportHeight);
}

void Renderer::OnInitialize()
{
	auto& RenderDevice = RenderDevice::Instance();

	ShaderCompiler ShaderCompiler;
	ShaderCompiler.SetShaderModel(D3D_SHADER_MODEL_6_6);
	ShaderCompiler.SetIncludeDirectory(Application::ExecutableDirectory / L"Shaders");

	Shaders::Compile(ShaderCompiler);
	Libraries::Compile(ShaderCompiler);

	AccelerationStructure = RaytracingAccelerationStructure(PathIntegrator_DXR_1_0::NumHitGroups);

	Manager = RaytracingAccelerationStructureManager(RenderDevice.GetDevice(), 6_MiB);

	PathIntegrator.Initialize(RenderDevice);
	ToneMapper.Initialize(RenderDevice);
	FSRFilter.Initialize(RenderDevice);

	Materials = Buffer(
		RenderDevice.GetDevice(),
		sizeof(HLSL::Material) * World::MaterialLimit,
		sizeof(HLSL::Material),
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_RESOURCE_FLAG_NONE);
	Materials.Initialize();
	pMaterials = Materials.GetCPUVirtualAddress<HLSL::Material>();

	Lights = Buffer(
		RenderDevice.GetDevice(),
		sizeof(HLSL::Light) * World::LightLimit,
		sizeof(HLSL::Light),
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_RESOURCE_FLAG_NONE);
	Lights.Initialize();
	pLights = Lights.GetCPUVirtualAddress<HLSL::Light>();
}

void Renderer::OnRender(World& World)
{
	RenderDevice::Instance().GetAdapter()->OnBeginFrame();

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
		ImGui::Combo("Quality", (int*)&FSRState.QualityMode, QualityModes, std::size(QualityModes));

		ImGui::SliderFloat("Sharpening attenuation", &FSRState.RCASAttenuation, 0.0f, 2.0f);

		ImGui::Text("Render resolution: %dx%d", RenderWidth, RenderHeight);
		ImGui::Text("Viewport resolution: %dx%d", ViewportWidth, ViewportHeight);
	}
	ImGui::End();

	World.ActiveCamera->AspectRatio = float(ViewportWidth) / float(ViewportHeight);
	auto& RenderDevice				= RenderDevice::Instance();

	UINT NumMaterials = 0, NumLights = 0;

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
		CommandContext& Copy = RenderDevice.GetDevice()->GetCopyContext1();
		Copy.OpenCommandList();

		// Update shader table
		PathIntegrator.UpdateShaderTable(AccelerationStructure, Copy);

		Copy.CloseCommandList();

		CopySyncPoint = Copy.Execute(false);
	}

	CommandSyncPoint ASBuildSyncPoint;
	if (!AccelerationStructure.empty())
	{
		CommandContext& AsyncCompute = RenderDevice.GetDevice()->GetAsyncComputeCommandContext();
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
		PathIntegrator.Reset();
	}

	// Begin recording graphics command

	CommandContext& Context = RenderDevice.GetDevice()->GetCommandContext();
	Context.GetCommandQueue()->WaitForSyncPoint(CopySyncPoint);
	Context.GetCommandQueue()->WaitForSyncPoint(ASBuildSyncPoint);
	Context.OpenCommandList();

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
	} g_GlobalConstants				  = {};
	g_GlobalConstants.Camera		  = GetHLSLCameraDesc(*World.ActiveCamera);
	g_GlobalConstants.Resolution	  = { float(ViewportWidth),
									  float(ViewportHeight),
									  1.0f / float(ViewportWidth),
									  1.0f / float(ViewportHeight) };
	g_GlobalConstants.MousePosition	  = { ViewportMouseX, ViewportMouseY };
	g_GlobalConstants.NumLights		  = NumLights;
	g_GlobalConstants.TotalFrameCount = Counter++;

	Allocation Allocation = Context.CpuConstantAllocator.Allocate(g_GlobalConstants);

	Context.BindResourceViewHeaps();

	if (!AccelerationStructure.empty())
	{
		// Enqueue ray tracing commands
		PathIntegrator.Render(
			PathIntegratorState,
			Allocation.GPUVirtualAddress,
			AccelerationStructure,
			Materials.GetGPUVirtualAddress(),
			Lights.GetGPUVirtualAddress(),
			Context);
	}

	ToneMapper.Apply(PathIntegrator.GetSRV(), Context);

	FSRFilter.Upscale(FSRState, ToneMapper.GetSRV(), Context);

	auto [pRenderTarget, RenderTargetView] = RenderDevice.GetCurrentBackBufferResource();

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

	RenderDevice.Present(false);

	RenderDevice::Instance().GetAdapter()->OnEndFrame();

	MainSyncPoint.WaitForCompletion();
}

void Renderer::OnResize(uint32_t Width, uint32_t Height)
{
	RenderDevice::Instance().GetDevice()->GetGraphicsQueue()->Flush();
	RenderDevice::Instance().Resize(Width, Height);
}

void Renderer::OnDestroy()
{
	RenderDevice::Instance().GetDevice()->GetGraphicsQueue()->Flush();
	RenderDevice::Instance().GetDevice()->GetAsyncComputeQueue()->Flush();
	RenderDevice::Instance().GetDevice()->GetCopyQueue1()->Flush();
	RenderDevice::Instance().GetDevice()->GetCopyQueue2()->Flush();
}

void Renderer::RequestCapture()
{
	/*auto SaveD3D12ResourceToDisk = [&](const std::filesystem::path& Path,
									   ID3D12Resource*				pResource,
									   D3D12_RESOURCE_STATES		Before,
									   D3D12_RESOURCE_STATES		After)
	{
		if (FAILED(DirectX::SaveWICTextureToFile(
				RenderDevice::Instance().GetGraphicsQueue(),
				pResource,
				GUID_ContainerFormatPng,
				Path.c_str(),
				Before,
				After,
				nullptr,
				nullptr,
				true)))
		{
			LOG_WARN("Failed to capture");
		}
		else
		{
			LOG_INFO("{} saved to disk!", Path.string());
		}
	};

	auto pTexture = m_ToneMapper.GetRenderTarget();
	SaveD3D12ResourceToDisk(
		Application::ExecutableDirectory / L"viewport.png",
		pTexture,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_RENDER_TARGET);

	auto [pRenderTarget, RenderTargetView] = RenderDevice::Instance().GetCurrentBackBufferResource();
	SaveD3D12ResourceToDisk(
		Application::ExecutableDirectory / L"swapchain.png",
		pRenderTarget,
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_PRESENT);*/
}

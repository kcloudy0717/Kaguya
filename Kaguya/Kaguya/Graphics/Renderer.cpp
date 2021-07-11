#include "Renderer.h"
#include "Core/Stopwatch.h"

#include "RenderDevice.h"
#include "Scene/Entity.h"

#include <wincodec.h> // GUID for different file formats, needed for ScreenGrab

#include "RendererRegistry.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

Renderer::Renderer()
	: Width(Application::Width())
	, Height(Application::Height())
	, ViewportMouseX(0)
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

	m_PathIntegrator.SetResolution(ViewportWidth, ViewportHeight);
	m_ToneMapper.SetResolution(ViewportWidth, ViewportHeight);
}

void Renderer::OnInitialize()
{
	auto& RenderDevice = RenderDevice::Instance();

	ShaderCompiler ShaderCompiler;
	ShaderCompiler.SetShaderModel(D3D_SHADER_MODEL_6_6);
	ShaderCompiler.SetIncludeDirectory(Application::ExecutableDirectory / L"Shaders");

	Shaders::Compile(ShaderCompiler);
	Libraries::Compile(ShaderCompiler);

	AccelerationStructure = RaytracingAccelerationStructure(PathIntegrator::NumHitGroups);

	m_PathIntegrator = PathIntegrator(RenderDevice::Instance());
	m_ToneMapper	 = ToneMapper(RenderDevice::Instance());

	Materials = Buffer(
		RenderDevice.GetDevice(),
		sizeof(HLSL::Material) * Scene::MaterialLimit,
		sizeof(HLSL::Material),
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_RESOURCE_FLAG_NONE);
	Materials.GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&pMaterials));

	Lights = Buffer(
		RenderDevice.GetDevice(),
		sizeof(HLSL::Light) * Scene::LightLimit,
		sizeof(HLSL::Light),
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_RESOURCE_FLAG_NONE);
	Lights.GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&pLights));
}

void Renderer::OnRender(Scene& Scene)
{
	auto& RenderDevice = RenderDevice::Instance();

	UINT NumMaterials = 0, NumLights = 0;

	AccelerationStructure.clear();
	Scene.Registry.view<Transform, MeshFilter, MeshRenderer>().each(
		[&](auto&& Transform, auto&& MeshFilter, auto&& MeshRenderer)
		{
			if (MeshFilter.Mesh)
			{
				AccelerationStructure.AddInstance(Transform, &MeshRenderer);
				pMaterials[NumMaterials++] = GetHLSLMaterialDesc(MeshRenderer.Material);
			}
		});
	Scene.Registry.view<Transform, Light>().each(
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
		m_PathIntegrator.UpdateShaderTable(AccelerationStructure, Copy);

		Copy.CloseCommandList();

		CopySyncPoint = Copy.Execute(false);
	}

	// TODO: remove this, create a dedicated scratch buffer memory allocator
	std::vector<Buffer> TrackedScratchBuffers;

	CommandSyncPoint ASBuildSyncPoint;
	if (!AccelerationStructure.empty())
	{
		CommandContext& AsyncCompute = RenderDevice.GetDevice()->GetAsyncComputeCommandContext();
		AsyncCompute.OpenCommandList();

		for (auto [i, meshRenderer] : enumerate(AccelerationStructure.MeshRenderers))
		{
			MeshFilter*						  meshFilter = meshRenderer->pMeshFilter;
			BottomLevelAccelerationStructure& BLAS		 = meshFilter->Mesh->BLAS;

			if (meshFilter->Mesh->BLASValid)
			{
				continue;
			}

			meshFilter->Mesh->BLASValid = true;

			ID3D12Resource* vertexBuffer = meshFilter->Mesh->VertexResource.GetResource();
			ID3D12Resource* indexBuffer	 = meshFilter->Mesh->IndexResource.GetResource();

			UINT64 scratchSize, resultSize;
			BLAS.ComputeMemoryRequirements(RenderDevice.GetDevice()->GetDevice5(), &scratchSize, &resultSize);

			// TODO: add a memory allocation scheme here, RTAS alignment is only 256, we can suballocate from a buffer
			// dedicated to build AS
			Buffer scratch = Buffer(
				RenderDevice.GetDevice(),
				scratchSize,
				0,
				D3D12_HEAP_TYPE_DEFAULT,
				D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
			meshFilter->Mesh->AccelerationStructure = ASBuffer(RenderDevice.GetDevice(), resultSize);

			BLAS.Generate(
				AsyncCompute.CommandListHandle.GetGraphicsCommandList6(),
				scratch,
				meshFilter->Mesh->AccelerationStructure);

			TrackedScratchBuffers.push_back(std::move(scratch));
		}

		if (!TrackedScratchBuffers.empty())
		{
			AsyncCompute.UAVBarrier(nullptr);
			AsyncCompute.FlushResourceBarriers();
		}

		AccelerationStructure.Build(AsyncCompute);

		AsyncCompute.CloseCommandList();

		ASBuildSyncPoint = AsyncCompute.Execute(false);
	}

	if (Scene.SceneState & ESceneState::SceneState_Update)
	{
		m_PathIntegrator.Reset();
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
		float4 Resolution;

		float2 MousePosition;

		unsigned int NumLights;
		unsigned int TotalFrameCount;
	} g_GlobalConstants				  = {};
	g_GlobalConstants.Camera		  = GetHLSLCameraDesc(Scene.Camera);
	g_GlobalConstants.Resolution	  = { float(ViewportWidth),
									  float(ViewportHeight),
									  1.0f / float(ViewportWidth),
									  1.0f / float(ViewportHeight) };
	g_GlobalConstants.MousePosition	  = { ViewportMouseX, ViewportMouseY };
	g_GlobalConstants.NumLights		  = NumLights;
	g_GlobalConstants.TotalFrameCount = Counter++;

	Allocation Allocation = Context.CpuConstantAllocator.Allocate(sizeof(GlobalConstants));
	std::memcpy(Allocation.CPUVirtualAddress, &g_GlobalConstants, sizeof(GlobalConstants));

	Context.BindResourceViewHeaps();

	if (!AccelerationStructure.empty())
	{
		// Enqueue ray tracing commands
		m_PathIntegrator.Render(
			Allocation.GPUVirtualAddress,
			AccelerationStructure,
			Materials.GetGPUVirtualAddress(),
			Lights.GetGPUVirtualAddress(),
			Context);
	}

	m_ToneMapper.Apply(m_PathIntegrator.GetSRV(), Context);

	auto [pRenderTarget, RenderTargetView] = RenderDevice.GetCurrentBackBufferResource();

	auto ResourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		pRenderTarget,
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET);
	Context->ResourceBarrier(1, &ResourceBarrier);
	{
		Viewport	= CD3DX12_VIEWPORT(0.0f, 0.0f, float(Width), float(Height));
		ScissorRect = CD3DX12_RECT(0, 0, Width, Height);

		Context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		Context->RSSetViewports(1, &Viewport);
		Context->RSSetScissorRects(1, &ScissorRect);
		Context->OMSetRenderTargets(1, &RenderTargetView, TRUE, nullptr);
		FLOAT white[] = { 1, 1, 1, 1 };
		Context->ClearRenderTargetView(RenderTargetView, white, 0, nullptr);

		// ImGui Render
		{
			PIXScopedEvent(Context.CommandListHandle.GetGraphicsCommandList(), 0, L"ImGui Render");

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

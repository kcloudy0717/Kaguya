#include "pch.h"
#include "Renderer.h"
#include "Core/Stopwatch.h"

#include "RenderDevice.h"
#include "Scene/Entity.h"

#include <wincodec.h> // GUID for different file formats, needed for ScreenGrab

#include "RendererRegistry.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

Renderer::Renderer()
	: RenderSystem(Application::Width(), Application::Height())
	, m_ViewportMouseX(0)
	, m_ViewportMouseY(0)
	, m_ViewportWidth(0)
	, m_ViewportHeight(0)
	, m_Viewport()
	, m_ScissorRect()
{
}

Renderer::~Renderer()
{
}

void Renderer::SetViewportMousePosition(float MouseX, float MouseY)
{
	m_ViewportMouseX = MouseX;
	m_ViewportMouseY = MouseY;
}

void Renderer::SetViewportResolution(uint32_t Width, uint32_t Height)
{
	if (!RenderDevice::IsValid())
	{
		return;
	}

	if (Width == 0 || Height == 0)
	{
		return;
	}

	m_ViewportWidth	 = Width;
	m_ViewportHeight = Height;

	m_PathIntegrator.SetResolution(m_ViewportWidth, m_ViewportHeight);
	m_ToneMapper.SetResolution(m_ViewportWidth, m_ViewportHeight);
}

void Renderer::Initialize()
{
	auto& RenderDevice = RenderDevice::Instance();

	ShaderCompiler shaderCompiler;
	shaderCompiler.SetShaderModel(D3D_SHADER_MODEL_6_6);
	shaderCompiler.SetIncludeDirectory(Application::ExecutableDirectory / L"Shaders");

	Shaders::Compile(shaderCompiler);
	Libraries::Compile(shaderCompiler);

	m_RaytracingAccelerationStructure = RaytracingAccelerationStructure(PathIntegrator::NumHitGroups);

	m_PathIntegrator = PathIntegrator(RenderDevice::Instance());
	m_Picking.Create();
	m_ToneMapper = ToneMapper(RenderDevice::Instance());

	m_Materials = Buffer(
		RenderDevice.GetDevice(),
		sizeof(HLSL::Material) * Scene::MAX_MATERIAL_SUPPORTED,
		sizeof(HLSL::Material),
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_RESOURCE_FLAG_NONE);
	ThrowIfFailed(m_Materials.GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&m_pMaterials)));

	m_Lights = Buffer(
		RenderDevice.GetDevice(),
		sizeof(HLSL::Light) * Scene::MAX_LIGHT_SUPPORTED,
		sizeof(HLSL::Light),
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_RESOURCE_FLAG_NONE);
	ThrowIfFailed(m_Lights.GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&m_pLights)));
}

void Renderer::Render(Scene& Scene)
{
	auto& RenderDevice = RenderDevice::Instance();

	UINT numMaterials = 0, numLights = 0;

	m_RaytracingAccelerationStructure.clear();
	Scene.Registry.view<Transform, MeshFilter, MeshRenderer>().each(
		[&](auto&& Transform, auto&& MeshFilter, auto&& MeshRenderer)
		{
			if (MeshFilter.Mesh)
			{
				m_RaytracingAccelerationStructure.AddInstance(Transform, &MeshRenderer);
				m_pMaterials[numMaterials++] = GetHLSLMaterialDesc(MeshRenderer.Material);
			}
		});
	Scene.Registry.view<Transform, Light>().each(
		[&](auto&& Transform, auto&& Light)
		{
			m_pLights[numLights++] = GetHLSLLightDesc(Transform, Light);
		});

	CommandSyncPoint CopySyncPoint;
	if (!m_RaytracingAccelerationStructure.empty())
	{
		CommandContext& Copy = RenderDevice.GetDevice()->GetCopyContext1();
		Copy.OpenCommandList();

		// Update shader table
		m_PathIntegrator.UpdateShaderTable(m_RaytracingAccelerationStructure, Copy);
		m_Picking.UpdateShaderTable(m_RaytracingAccelerationStructure, Copy);

		Copy.CloseCommandList();

		CopySyncPoint = Copy.Execute(false);
	}

	std::vector<Buffer> TrackedScratchBuffers;

	CommandSyncPoint ASBuildSyncPoint;
	if (!m_RaytracingAccelerationStructure.empty())
	{
		CommandContext& AsyncCompute = RenderDevice.GetDevice()->GetAsyncComputeCommandContext();
		AsyncCompute.OpenCommandList();

		for (auto [i, meshRenderer] : enumerate(m_RaytracingAccelerationStructure.MeshRenderers))
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

		AsyncCompute.UAVBarrier(nullptr);
		AsyncCompute.FlushResourceBarriers();

		m_RaytracingAccelerationStructure.Build(AsyncCompute);

		AsyncCompute.CloseCommandList();

		ASBuildSyncPoint = AsyncCompute.Execute(true);
	}

	if (Scene.SceneState & Scene::SCENE_STATE_UPDATED)
	{
		m_PathIntegrator.Reset();
	}

	// Begin recording graphics command

	CommandContext& Context = RenderDevice.GetDevice()->GetCommandContext();
	Context.GetCommandQueue()->WaitForSyncPoint(CopySyncPoint);
	Context.GetCommandQueue()->WaitForSyncPoint(ASBuildSyncPoint);
	Context.OpenCommandList();

	struct GlobalConstants
	{
		HLSL::Camera Camera;

		// x, y = Resolution
		// z, w = 1 / Resolution
		float4 Resolution;

		float2 MousePosition;

		uint TotalFrameCount;
		uint NumLights;
	} g_GlobalConstants				  = {};
	g_GlobalConstants.Camera		  = GetHLSLCameraDesc(Scene.Camera);
	g_GlobalConstants.Resolution	  = { float(m_ViewportWidth),
									  float(m_ViewportHeight),
									  1.0f / float(m_ViewportWidth),
									  1.0f / float(m_ViewportHeight) };
	g_GlobalConstants.MousePosition	  = { m_ViewportMouseX, m_ViewportMouseY };
	g_GlobalConstants.TotalFrameCount = static_cast<unsigned int>(Statistics::TotalFrameCount);
	g_GlobalConstants.NumLights		  = numLights;

	Allocation Allocation = Context.CpuConstantAllocator.Allocate(sizeof(GlobalConstants));
	std::memcpy(Allocation.CPUVirtualAddress, &g_GlobalConstants, sizeof(GlobalConstants));

	Context.BindResourceViewHeaps();

	if (!m_RaytracingAccelerationStructure.empty())
	{
		// Enqueue ray tracing commands
		m_PathIntegrator.Render(
			Allocation.GPUVirtualAddress,
			m_RaytracingAccelerationStructure,
			m_Materials.GetGPUVirtualAddress(),
			m_Lights.GetGPUVirtualAddress(),
			Context);

		m_Picking.ShootPickingRay(Allocation.GPUVirtualAddress, m_RaytracingAccelerationStructure, Context);
	}

	m_ToneMapper.Apply(m_PathIntegrator.GetSRV(), Context);

	auto [pRenderTarget, RenderTargetView] = RenderDevice.GetCurrentBackBufferResource();

	auto ResourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		pRenderTarget,
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET);
	Context->ResourceBarrier(1, &ResourceBarrier);
	{
		m_Viewport	  = CD3DX12_VIEWPORT(0.0f, 0.0f, float(m_Width), float(m_Height));
		m_ScissorRect = CD3DX12_RECT(0, 0, m_Width, m_Height);

		Context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		Context->RSSetViewports(1, &m_Viewport);
		Context->RSSetScissorRects(1, &m_ScissorRect);
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

	RenderDevice.Present(Settings::VSync);

	MainSyncPoint.WaitForCompletion();
}

void Renderer::Resize(uint32_t Width, uint32_t Height)
{
	RenderDevice::Instance().GetDevice()->GetGraphicsQueue()->Flush();
	RenderDevice::Instance().Resize(Width, Height);
}

void Renderer::Destroy()
{
	RenderDevice::Instance().GetDevice()->GetGraphicsQueue()->Flush();
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

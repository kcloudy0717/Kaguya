#include "pch.h"
#include "Renderer.h"
#include "Core/Window.h"
#include "Core/Time.h"

#include "RenderDevice.h"
#include "Scene/Entity.h"

#include <wincodec.h> // GUID for different file formats, needed for ScreenGrab
#include <ScreenGrab.h> // DirectX::SaveWICTextureToFile

#include "RendererRegistry.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

struct SystemConstants
{
	HLSL::Camera Camera;

	// x, y = Resolution
	// z, w = 1 / Resolution
	float4 Resolution;

	float2 MousePosition;

	uint TotalFrameCount;
	uint NumLights;
};

Renderer::Renderer()
	: RenderSystem(Application::Window.GetWindowWidth(), Application::Window.GetWindowHeight())
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

Descriptor Renderer::GetViewportDescriptor()
{
	return m_ToneMapper.GetSRV();
}

Entity Renderer::GetSelectedEntity()
{
	return m_Picking.GetSelectedEntity().value_or(Entity());
}

void Renderer::SetViewportMousePosition(float MouseX, float MouseY)
{
	m_ViewportMouseX = MouseX;
	m_ViewportMouseY = MouseY;
}

void Renderer::SetViewportResolution(uint32_t Width, uint32_t Height)
{
	if (Width == 0 || Height == 0)
		return;

	m_ViewportWidth = Width;
	m_ViewportHeight = Height;

	m_PathIntegrator.SetResolution(m_ViewportWidth, m_ViewportHeight);
	m_ToneMapper.SetResolution(m_ViewportWidth, m_ViewportHeight);
}

void Renderer::Initialize()
{
	auto& RenderDevice = RenderDevice::Instance();

	Shaders::Compile(RenderDevice.ShaderCompiler);
	Libraries::Compile(RenderDevice.ShaderCompiler);

	RenderDevice::Instance().CreateCommandContexts(5);

	m_RaytracingAccelerationStructure.Create(PathIntegrator::NumHitGroups);

	m_PathIntegrator.Create();
	m_Picking.Create();
	m_ToneMapper.Create();

	D3D12MA::ALLOCATION_DESC allocationDesc = {};
	allocationDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

	m_Materials = RenderDevice.CreateBuffer(&allocationDesc, sizeof(HLSL::Material) * Scene::MAX_MATERIAL_SUPPORTED, D3D12_RESOURCE_FLAG_NONE, 0, D3D12_RESOURCE_STATE_GENERIC_READ);
	ThrowIfFailed(m_Materials->pResource->Map(0, nullptr, reinterpret_cast<void**>(&m_pMaterials)));

	m_Lights = RenderDevice.CreateBuffer(&allocationDesc, sizeof(HLSL::Light) * Scene::MAX_LIGHT_SUPPORTED, D3D12_RESOURCE_FLAG_NONE, 0, D3D12_RESOURCE_STATE_GENERIC_READ);
	ThrowIfFailed(m_Lights->pResource->Map(0, nullptr, reinterpret_cast<void**>(&m_pLights)));
}

void Renderer::Render(const Time& Time, Scene& Scene)
{
	auto& RenderDevice = RenderDevice::Instance();

	UINT numMaterials = 0, numLights = 0;

	m_RaytracingAccelerationStructure.clear();
	Scene.Registry.view<MeshFilter, MeshRenderer>().each([&](auto&& MeshFilter, auto&& MeshRenderer)
	{
		if (MeshFilter.Mesh)
		{
			m_RaytracingAccelerationStructure.AddInstance(&MeshRenderer);
			m_pMaterials[numMaterials++] = GetHLSLMaterialDesc(MeshRenderer.Material);
		}
	});
	Scene.Registry.view<Transform, Light>().each([&](auto&& Transform, auto&& Light)
	{
		m_pLights[numLights++] = GetHLSLLightDesc(Transform, Light);
	});

	if (!m_RaytracingAccelerationStructure.empty())
	{
		auto& AsyncComputeContext = RenderDevice.GetDefaultAsyncComputeContext();
		AsyncComputeContext.Reset(RenderDevice.ComputeFenceValue, RenderDevice.ComputeFence->GetCompletedValue(), &RenderDevice.ComputeQueue);

		m_RaytracingAccelerationStructure.Build(AsyncComputeContext);

		CommandList* pCommandContexts[] = { &AsyncComputeContext };
		RenderDevice.ExecuteAsyncComputeContexts(1, pCommandContexts);
		RenderDevice.ComputeQueue->Signal(RenderDevice.ComputeFence.Get(), RenderDevice.ComputeFenceValue);
		RenderDevice.GraphicsQueue->Wait(RenderDevice.ComputeFence.Get(), RenderDevice.ComputeFenceValue);
		RenderDevice.ComputeFenceValue++;
	}

	if (Scene.SceneState & Scene::SCENE_STATE_UPDATED)
	{
		m_PathIntegrator.Reset();
	}

	// Begin recording graphics command
	auto& GraphicsContext = RenderDevice.GetDefaultGraphicsContext();
	GraphicsContext.Reset(RenderDevice::Instance().GraphicsFenceValue, RenderDevice::Instance().GraphicsFence->GetCompletedValue(), &RenderDevice.GraphicsQueue);

	SystemConstants g_SystemConstants = {};
	g_SystemConstants.Camera = GetHLSLCameraDesc(Scene.Camera);
	g_SystemConstants.Resolution = { float(m_ViewportWidth), float(m_ViewportHeight), 1.0f / float(m_ViewportWidth), 1.0f / float(m_ViewportHeight) };
	g_SystemConstants.MousePosition = { m_ViewportMouseX, m_ViewportMouseY };
	g_SystemConstants.TotalFrameCount = static_cast<unsigned int>(Statistics::TotalFrameCount);
	g_SystemConstants.NumLights = numLights;

	GraphicsResource sceneConstantBuffer = RenderDevice::Instance().Device.GraphicsMemory()->AllocateConstant(g_SystemConstants);

	RenderDevice::Instance().BindGlobalDescriptorHeap(GraphicsContext);

	if (!m_RaytracingAccelerationStructure.empty())
	{
		// Update shader table
		m_PathIntegrator.UpdateShaderTable(m_RaytracingAccelerationStructure, GraphicsContext);

		// Enqueue ray tracing commands
		m_PathIntegrator.Render(sceneConstantBuffer.GpuAddress(),
			m_RaytracingAccelerationStructure,
			m_Materials->pResource->GetGPUVirtualAddress(),
			m_Lights->pResource->GetGPUVirtualAddress(),
			GraphicsContext);

		m_Picking.UpdateShaderTable(m_RaytracingAccelerationStructure, GraphicsContext);
		m_Picking.ShootPickingRay(sceneConstantBuffer.GpuAddress(), m_RaytracingAccelerationStructure, GraphicsContext);

		m_ToneMapper.Apply(m_PathIntegrator.GetSRV(), GraphicsContext);
	}

	auto pBackBuffer = RenderDevice::Instance().GetCurrentBackBuffer();
	auto rtv = RenderDevice::Instance().GetCurrentBackBufferRenderTargetView();

	GraphicsContext.TransitionBarrier(pBackBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
	{
		m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, float(m_Width), float(m_Height));
		m_ScissorRect = CD3DX12_RECT(0, 0, m_Width, m_Height);

		GraphicsContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		GraphicsContext->RSSetViewports(1, &m_Viewport);
		GraphicsContext->RSSetScissorRects(1, &m_ScissorRect);
		GraphicsContext->OMSetRenderTargets(1, &rtv.CpuHandle, TRUE, nullptr);
		GraphicsContext->ClearRenderTargetView(rtv.CpuHandle, DirectX::Colors::White, 0, nullptr);

		// ImGui Render
		{
			PIXScopedEvent(GraphicsContext.GetApiHandle(), 0, L"ImGui Render");

			ImGui::Render();
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), GraphicsContext);
		}
	}
	GraphicsContext.TransitionBarrier(pBackBuffer, D3D12_RESOURCE_STATE_PRESENT);

	CommandList* pCommandLists[] = { &GraphicsContext };
	RenderDevice::Instance().ExecuteGraphicsContexts(1, pCommandLists);
	RenderDevice::Instance().Present(Settings::VSync);
	RenderDevice::Instance().Device.GraphicsMemory()->Commit(RenderDevice::Instance().GraphicsQueue);
	RenderDevice::Instance().FlushGraphicsQueue();
}

void Renderer::Resize(uint32_t Width, uint32_t Height)
{
	RenderDevice::Instance().FlushGraphicsQueue();
	RenderDevice::Instance().Resize(Width, Height);
}

void Renderer::Destroy()
{

}

void Renderer::RequestCapture()
{
	auto SaveD3D12ResourceToDisk = [&](const std::filesystem::path& Path, ID3D12Resource* pResource, D3D12_RESOURCE_STATES Before, D3D12_RESOURCE_STATES After)
	{
		if (FAILED(DirectX::SaveWICTextureToFile(RenderDevice::Instance().GraphicsQueue, pResource,
			GUID_ContainerFormatPng, Path.c_str(),
			Before, After, nullptr, nullptr, true)))
		{
			LOG_WARN("Failed to capture");
		}
		else
		{
			LOG_INFO("{} saved to disk!", Path.string());
		}
	};

	auto pTexture = m_ToneMapper.GetRenderTarget();
	SaveD3D12ResourceToDisk(Application::ExecutableFolderPath / L"viewport.png",
		pTexture,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_RENDER_TARGET);

	auto pBackBuffer = RenderDevice::Instance().GetCurrentBackBuffer();
	SaveD3D12ResourceToDisk(Application::ExecutableFolderPath / L"swapchain.png",
		pBackBuffer,
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_PRESENT);
}
#include "pch.h"
#include "Renderer.h"
#include "Core/Window.h"
#include "Core/Stopwatch.h"

#include "RenderDevice.h"
#include "Scene/Entity.h"

#include <wincodec.h>	// GUID for different file formats, needed for ScreenGrab
#include <ScreenGrab.h> // DirectX::SaveWICTextureToFile

#include "RendererRegistry.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

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

void Renderer::SetViewportMousePosition(float MouseX, float MouseY)
{
	m_ViewportMouseX = MouseX;
	m_ViewportMouseY = MouseY;
}

void Renderer::SetViewportResolution(uint32_t Width, uint32_t Height)
{
	if (Width == 0 || Height == 0)
		return;

	m_ViewportWidth	 = Width;
	m_ViewportHeight = Height;

	m_PathIntegrator.SetResolution(m_ViewportWidth, m_ViewportHeight);
	m_ToneMapper.SetResolution(m_ViewportWidth, m_ViewportHeight);
}

void Renderer::Initialize()
{
	auto& RenderDevice = RenderDevice::Instance();

	ShaderCompiler shaderCompiler;
	shaderCompiler.SetIncludeDirectory(Application::ExecutableDirectory / L"Shaders");

	Shaders::Compile(shaderCompiler);
	Libraries::Compile(shaderCompiler);

	m_GraphicsFence		 = Fence(RenderDevice.GetDevice());
	m_ComputeFence		 = Fence(RenderDevice.GetDevice());
	m_GraphicsFenceValue = m_ComputeFenceValue = 1;

	m_GraphicsCommandList = CommandList(RenderDevice.GetDevice(), D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_ComputeCommandList  = CommandList(RenderDevice.GetDevice(), D3D12_COMMAND_LIST_TYPE_COMPUTE);

	m_RaytracingAccelerationStructure = RaytracingAccelerationStructure(PathIntegrator::NumHitGroups);

	m_PathIntegrator = PathIntegrator(RenderDevice::Instance());
	m_Picking.Create();
	m_ToneMapper = ToneMapper(RenderDevice::Instance());

	D3D12MA::ALLOCATION_DESC allocationDesc = {};
	allocationDesc.HeapType					= D3D12_HEAP_TYPE_UPLOAD;

	m_Materials = RenderDevice.CreateBuffer(
		&allocationDesc,
		sizeof(HLSL::Material) * Scene::MAX_MATERIAL_SUPPORTED,
		D3D12_RESOURCE_FLAG_NONE,
		0,
		D3D12_RESOURCE_STATE_GENERIC_READ);
	ThrowIfFailed(m_Materials->pResource->Map(0, nullptr, reinterpret_cast<void**>(&m_pMaterials)));

	m_Lights = RenderDevice.CreateBuffer(
		&allocationDesc,
		sizeof(HLSL::Light) * Scene::MAX_LIGHT_SUPPORTED,
		D3D12_RESOURCE_FLAG_NONE,
		0,
		D3D12_RESOURCE_STATE_GENERIC_READ);
	ThrowIfFailed(m_Lights->pResource->Map(0, nullptr, reinterpret_cast<void**>(&m_pLights)));
}

void Renderer::Render(Scene& Scene)
{
	auto& RenderDevice = RenderDevice::Instance();

	UINT numMaterials = 0, numLights = 0;

	m_RaytracingAccelerationStructure.clear();
	Scene.Registry.view<MeshFilter, MeshRenderer>().each(
		[&](auto&& MeshFilter, auto&& MeshRenderer)
		{
			if (MeshFilter.Mesh)
			{
				m_RaytracingAccelerationStructure.AddInstance(&MeshRenderer);
				m_pMaterials[numMaterials++] = GetHLSLMaterialDesc(MeshRenderer.Material);
			}
		});
	Scene.Registry.view<Transform, Light>().each(
		[&](auto&& Transform, auto&& Light)
		{
			m_pLights[numLights++] = GetHLSLLightDesc(Transform, Light);
		});

	if (!m_RaytracingAccelerationStructure.empty())
	{
		m_ComputeCommandList.Reset(
			m_ComputeFenceValue,
			m_ComputeFence->GetCompletedValue(),
			&RenderDevice.GetComputeQueue());

		m_RaytracingAccelerationStructure.Build(m_ComputeCommandList);

		CommandList* pCommandContexts[] = { &m_ComputeCommandList };
		RenderDevice.ExecuteAsyncComputeContexts(1, pCommandContexts);

		RenderDevice.GetComputeQueue()->Signal(m_ComputeFence, m_ComputeFenceValue);
		RenderDevice.GetGraphicsQueue()->Wait(m_ComputeFence, m_ComputeFenceValue);
		m_ComputeFenceValue++;
	}

	if (Scene.SceneState & Scene::SCENE_STATE_UPDATED)
	{
		m_PathIntegrator.Reset();
	}

	// Begin recording graphics command
	m_GraphicsCommandList.Reset(
		m_GraphicsFenceValue,
		m_GraphicsFence->GetCompletedValue(),
		&RenderDevice.GetGraphicsQueue());

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

	GraphicsResource sceneConstantBuffer = RenderDevice.GraphicsMemory()->AllocateConstant(g_GlobalConstants);

	RenderDevice.BindResourceViewHeaps(m_GraphicsCommandList);

	if (!m_RaytracingAccelerationStructure.empty())
	{
		// Update shader table
		m_PathIntegrator.UpdateShaderTable(m_RaytracingAccelerationStructure, m_GraphicsCommandList);

		// Enqueue ray tracing commands
		m_PathIntegrator.Render(
			sceneConstantBuffer.GpuAddress(),
			m_RaytracingAccelerationStructure,
			m_Materials->pResource->GetGPUVirtualAddress(),
			m_Lights->pResource->GetGPUVirtualAddress(),
			m_GraphicsCommandList);

		m_Picking.UpdateShaderTable(m_RaytracingAccelerationStructure, m_GraphicsCommandList);
		m_Picking.ShootPickingRay(
			sceneConstantBuffer.GpuAddress(),
			m_RaytracingAccelerationStructure,
			m_GraphicsCommandList);

		m_ToneMapper.Apply(m_PathIntegrator.GetSRV(), m_GraphicsCommandList);
	}

	auto [pRenderTarget, RenderTargetView] = RenderDevice.GetCurrentBackBufferResource();

	m_GraphicsCommandList.TransitionBarrier(pRenderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET);
	{
		m_Viewport	  = CD3DX12_VIEWPORT(0.0f, 0.0f, float(m_Width), float(m_Height));
		m_ScissorRect = CD3DX12_RECT(0, 0, m_Width, m_Height);

		m_GraphicsCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_GraphicsCommandList->RSSetViewports(1, &m_Viewport);
		m_GraphicsCommandList->RSSetScissorRects(1, &m_ScissorRect);
		m_GraphicsCommandList->OMSetRenderTargets(1, &RenderTargetView.CpuHandle, TRUE, nullptr);
		FLOAT white[] = { 1, 1, 1, 1 };
		m_GraphicsCommandList->ClearRenderTargetView(RenderTargetView.CpuHandle, white, 0, nullptr);

		// ImGui Render
		{
			PIXScopedEvent(m_GraphicsCommandList.GetCommandList(), 0, L"ImGui Render");

			ImGui::Render();
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_GraphicsCommandList);
		}
	}
	m_GraphicsCommandList.TransitionBarrier(pRenderTarget, D3D12_RESOURCE_STATE_PRESENT);

	CommandList* pCommandLists[] = { &m_GraphicsCommandList };
	RenderDevice.ExecuteGraphicsContexts(1, pCommandLists);
	RenderDevice.Present(Settings::VSync);

	UINT64 value = ++m_GraphicsFenceValue;
	ThrowIfFailed(RenderDevice::Instance().GetGraphicsQueue()->Signal(m_GraphicsFence, value));
	m_GraphicsFence->SetEventOnCompletion(value, nullptr);
}

void Renderer::Resize(uint32_t Width, uint32_t Height)
{
	UINT64 value = ++m_GraphicsFenceValue;
	ThrowIfFailed(RenderDevice::Instance().GetGraphicsQueue()->Signal(m_GraphicsFence, value));
	m_GraphicsFence->SetEventOnCompletion(value, nullptr);
	RenderDevice::Instance().Resize(Width, Height);
}

void Renderer::Destroy()
{
	UINT64 value = ++m_GraphicsFenceValue;
	ThrowIfFailed(RenderDevice::Instance().GetGraphicsQueue()->Signal(m_GraphicsFence, value));
	m_GraphicsFence->SetEventOnCompletion(value, nullptr);

	value = ++m_ComputeFenceValue;
	ThrowIfFailed(RenderDevice::Instance().GetComputeQueue()->Signal(m_ComputeFence, value));
	m_ComputeFence->SetEventOnCompletion(value, nullptr);
}

void Renderer::RequestCapture()
{
	auto SaveD3D12ResourceToDisk = [&](const std::filesystem::path& Path,
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
		D3D12_RESOURCE_STATE_PRESENT);
}

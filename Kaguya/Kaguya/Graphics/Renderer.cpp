#include "Renderer.h"
#include "World/Entity.h"
#include "RendererRegistry.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

void Renderer::OnSetViewportResolution(uint32_t Width, uint32_t Height)
{
	if (Width == 0 || Height == 0)
	{
		return;
	}

	ViewportWidth  = Width;
	ViewportHeight = Height;

	SetViewportResolution(Width, Height);

	RenderGraph.SetResolution(RenderWidth, RenderHeight, ViewportWidth, ViewportHeight);
	RenderGraph.GetRegistry().RealizeResources();
	RenderGraph.ValidViewport = false;
}

void Renderer::OnInitialize()
{
	Initialize();
}

void Renderer::OnRender()
{
	pWorld->ActiveCamera->AspectRatio = float(RenderWidth) / float(RenderHeight);

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

	if (ImGui::Begin("Render Graph"))
	{
		for (const auto& RenderPass : RenderGraph)
		{
		}
	}
	ImGui::End();

	CommandContext& Context = RenderCore::pAdapter->GetDevice()->GetCommandContext();
	Context.OpenCommandList();
	Context.BindResourceViewHeaps();

	Render(Context);

	auto [pRenderTarget, RenderTargetView] = RenderCore::pSwapChain->GetCurrentBackBufferResource();

	auto Barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		pRenderTarget,
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET);
	Context->ResourceBarrier(1, &Barrier);
	{
		D3D12_VIEWPORT Viewport =
			CD3DX12_VIEWPORT(0.0f, 0.0f, float(Application::GetWidth()), float(Application::GetHeight()));
		D3D12_RECT ScissorRect = CD3DX12_RECT(0, 0, Application::GetWidth(), Application::GetHeight());

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
	Barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		pRenderTarget,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT);
	Context->ResourceBarrier(1, &Barrier);

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

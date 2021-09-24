#include "Renderer.h"
#include "RendererRegistry.h"

void Renderer::OnSetViewportResolution(uint32_t Width, uint32_t Height)
{
	if (Width == 0 || Height == 0)
	{
		return;
	}

	SetViewportResolution(Width, Height);
}

void Renderer::OnInitialize()
{
	Initialize();
}

void Renderer::OnRender()
{
	pWorld->ActiveCamera->AspectRatio = float(Resolution.RenderWidth) / float(Resolution.RenderHeight);

	RenderCore::Device->OnBeginFrame();

	if (ImGui::Begin("GPU Timing"))
	{
		for (const auto& iter : D3D12Profiler::Data)
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

	D3D12CommandContext& Context = RenderCore::Device->GetDevice()->GetCommandContext();
	Context.OpenCommandList();

	Render(Context);

	auto [pRenderTarget, RenderTargetView] = RenderCore::SwapChain->GetCurrentBackBufferResource();

	auto Barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		pRenderTarget,
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET);
	Context->ResourceBarrier(1, &Barrier);
	{
		auto Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, float(Application::GetWidth()), float(Application::GetHeight()));
		auto ScissorRect = CD3DX12_RECT(0, 0, Application::GetWidth(), Application::GetHeight());

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
	// TODO: Present submits work, and if you don't Signal() after that, you can't safely know when to destroy/resize
	// the swapchain, move signaling after present
	D3D12CommandSyncPoint MainSyncPoint = Context.Execute(false);

	RenderCore::SwapChain->Present(false);

	RenderCore::Device->OnEndFrame();

	MainSyncPoint.WaitForCompletion();
}

void Renderer::OnResize(uint32_t Width, uint32_t Height)
{
	RenderCore::Device->GetDevice()->GetGraphicsQueue()->WaitIdle();
	RenderCore::SwapChain->Resize(Width, Height);
}

void Renderer::OnDestroy()
{
}

#include "Renderer.h"
#include "RendererRegistry.h"

using Microsoft::WRL::ComPtr;

Renderer::Renderer(HWND HWnd)
	: SwapChain(RenderCore::Device, HWnd)
	, Allocator(64 * 1024)
	, Registry(Scheduler)
{
	SwapChain.Initialize();
}

void Renderer::OnSetViewportResolution(uint32_t Width, uint32_t Height)
{
	// Clamp at 4k
	Width  = std::clamp<uint32_t>(Width, 0, 3840);
	Height = std::clamp<uint32_t>(Height, 0, 2160);
	if (Width == 0 || Height == 0)
	{
		return;
	}

	ValidViewport = false;
	SetViewportResolution(Width, Height);
}

void Renderer::OnInitialize()
{
	Initialize();
}

void Renderer::OnDestroy()
{
	RenderCore::Device->GetDevice()->GetGraphicsQueue()->WaitIdle();
	RenderCore::Device->GetDevice()->GetAsyncComputeQueue()->WaitIdle();
	RenderCore::Device->GetDevice()->GetCopyQueue1()->WaitIdle();
	RenderCore::Device->GetDevice()->GetCopyQueue2()->WaitIdle();
	Destroy();
}

void Renderer::OnRender(World* World)
{
	World->ActiveCamera->AspectRatio = float(Resolution.RenderWidth) / float(Resolution.RenderHeight);

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
			ImGui::Text("%s: %.2fms (%.2fms max)", iter.Name, iter.AverageTime, iter.MaxTime);
			ImGui::SameLine();
			ImGui::NewLine();
		}

		constexpr size_t NumFrames				   = 128;
		static float	 FrameTimeArray[NumFrames] = { 0 };

		// track highest frame rate and determine the max value of the graph based on the measured highest value
		static float	Recent	 = 0.0f;
		constexpr float MaxFps[] = { 800.0f, 240.0f, 120.0f, 90.0f, 60.0f, 45.0f, 30.0f,
									 15.0f,	 10.0f,	 5.0f,	 4.0f,	3.0f,  2.0f,  1.0f };

		static float MaxFpsScale[std::size(MaxFps)] = { 0 }; // ms
		for (size_t i = 0; i < std::size(MaxFps); ++i)
		{
			MaxFpsScale[i] = 1000.0f / MaxFps[i];
		}

		// scrolling data and average FPS computing
		if (!D3D12Profiler::Data.empty())
		{
			Recent						  = 0;
			FrameTimeArray[NumFrames - 1] = static_cast<float>(D3D12Profiler::Data.begin()->AverageTime);
			for (size_t i = 0; i < NumFrames - 1; i++)
			{
				FrameTimeArray[i] = FrameTimeArray[i + 1];
			}
			Recent = std::max(Recent, FrameTimeArray[NumFrames - 1]);
		}

		// find the index of the FrameTimeGraphMaxValue as the next higher-than-recent-highest-frame-time in the
		// pre-determined value list
		size_t ScaleIndex = 0;
		for (size_t i = 0; i < std::size(MaxFps); ++i)
		{
			if (Recent < MaxFpsScale[i]) // MaxFpsScale are in increasing order
			{
				ScaleIndex = std::min<size_t>(std::size(MaxFps) - 1, i + 1);
				break;
			}
		}
		ImGui::PlotLines("", FrameTimeArray, NumFrames, 0, "", 0.0f, MaxFpsScale[ScaleIndex], ImVec2(0, 80));
	}
	ImGui::End();

	D3D12CommandContext& Context = RenderCore::Device->GetDevice()->GetCommandContext();
	Context.OpenCommandList();
	{
		D3D12ScopedEvent(Context, "Render");
		Render(World, Context);

		auto [pRenderTarget, RenderTargetView] = SwapChain.GetCurrentBackBufferResource();

		auto Barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			pRenderTarget,
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET);
		Context->ResourceBarrier(1, &Barrier);
		{
			D3D12_VIEWPORT Viewport	   = SwapChain.GetViewport();
			D3D12_RECT	   ScissorRect = SwapChain.GetScissorRect();

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
	}
	Context.CloseCommandList();

	RendererPresent Present(Context);
	SwapChain.Present(false, Present);

	RenderCore::Device->OnEndFrame();
	++FrameIndex;
}

void Renderer::OnResize(uint32_t Width, uint32_t Height)
{
	SwapChain.Resize(Width, Height);
}

void* Renderer::GetViewportDescriptor() const noexcept
{
	if (ValidViewport)
	{
		return Viewport;
	}

	return nullptr;
}

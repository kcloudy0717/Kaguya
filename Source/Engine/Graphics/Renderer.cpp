#include "Renderer.h"
#include "RendererRegistry.h"

using Microsoft::WRL::ComPtr;

Renderer::Renderer(Window* MainWindow)
	: MainWindow(MainWindow)
	, SwapChain(RenderCore::Device, MainWindow->GetWindowHandle())
	, Allocator(64 * 1024)
{
}

void Renderer::OnInitialize()
{
	Initialize();
}

void Renderer::OnDestroy()
{
	RenderCore::Device->WaitIdle();
	Destroy();
}

void Renderer::OnRender(World* World)
{
	/*static bool CaptureOnce = false;
	if (!CaptureOnce)
	{
		RenderCore::Device->BeginCapture(Application::ExecutableDirectory / "GPU 0.wpix");
	}*/

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
		constexpr float MaxFps[] = { 800.0f, 240.0f, 120.0f, 90.0f, 60.0f, 45.0f, 30.0f, 15.0f, 10.0f, 5.0f, 4.0f, 3.0f, 2.0f, 1.0f };

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
	Context.Open();
	{
		ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse);
		{
			bool IsHovered = ImGui::IsWindowHovered();

			ImGuizmo::SetDrawlist();
			ImGuizmo::SetRect(
				ImGui::GetWindowPos().x,
				ImGui::GetWindowPos().y,
				ImGui::GetWindowWidth(),
				ImGui::GetWindowHeight());

			auto ViewportOffset = ImGui::GetCursorPos(); // includes tab bar
			auto ViewportPos	= ImGui::GetWindowPos();
			auto ViewportSize	= ImGui::GetContentRegionAvail();
			auto WindowSize		= ImGui::GetWindowSize();

			RECT Rect	= {};
			Rect.left	= static_cast<LONG>(ViewportPos.x + ViewportOffset.x);
			Rect.top	= static_cast<LONG>(ViewportPos.y + ViewportOffset.y);
			Rect.right	= static_cast<LONG>(Rect.left + WindowSize.x);
			Rect.bottom = static_cast<LONG>(Rect.top + WindowSize.y);

			// Clamp at 4k
			View.Width						 = std::clamp<uint32_t>(static_cast<int>(ViewportSize.x), 1, 3840);
			View.Height						 = std::clamp<uint32_t>(static_cast<int>(ViewportSize.y), 1, 2160);
			World->ActiveCamera->AspectRatio = static_cast<float>(View.Width) / static_cast<float>(View.Height);

			D3D12ScopedEvent(Context, "Render");
			Render(World, Context);

			// Viewport must be valid after Render
			assert(Viewport);
			ImGui::Image(Viewport, ViewportSize);

			if (ImGui::IsMouseDown(ImGuiMouseButton_Right) && IsHovered)
			{
				Application::InputManager.DisableCursor(MainWindow->GetWindowHandle());
			}
			else if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
			{
				Application::InputManager.EnableCursor();
			}

			// Camera operates in raw input mode
			if (!Application::InputManager.CursorEnabled)
			{
				Application::InputManager.RawInputEnabled = true;
				MainWindow->SetRawInput(true);
				ClipCursor(&Rect);
			}
			else
			{
				Application::InputManager.RawInputEnabled = false;
				MainWindow->SetRawInput(false);
				ClipCursor(nullptr);
			}
		}
		ImGui::End();

		WorldWindow.SetContext(World);
		WorldWindow.Render();
		AssetWindow.SetContext(World);
		AssetWindow.Render();
		ConsoleWindow.Render();
		InspectorWindow.SetContext(World, WorldWindow.GetSelectedActor());
		InspectorWindow.Render();

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
				ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), Context.GetGraphicsCommandList());
			}
		}
		Barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			pRenderTarget,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT);
		Context->ResourceBarrier(1, &Barrier);
	}
	Context.Close();

	RendererPresent Present(Context);
	SwapChain.Present(true, Present);

	RenderCore::Device->OnEndFrame();
	++FrameIndex;
	/*if (!CaptureOnce)
	{
		CaptureOnce = true;
		RenderCore::Device->EndCapture();
	}*/
}

void Renderer::OnResize(uint32_t Width, uint32_t Height)
{
	SwapChain.Resize(Width, Height);
}

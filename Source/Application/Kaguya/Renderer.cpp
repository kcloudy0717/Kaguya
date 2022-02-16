#include "Renderer.h"
#include "RendererRegistry.h"

Renderer::Renderer(RHI::D3D12Device* Device, ShaderCompiler* Compiler, Window* MainWindow)
	: Device(Device)
	, Compiler(Compiler)
	, MainWindow(MainWindow)
	, SwapChain(Device, MainWindow->GetWindowHandle())
	, Allocator(64 * 1024)
{
}

void Renderer::OnInitialize()
{
	Initialize();
}

void Renderer::OnDestroy()
{
	Device->WaitIdle();
	Destroy();
}

void Renderer::OnRender(World* World)
{
	/*static bool CaptureOnce = false;
	if (!CaptureOnce)
	{
		RenderCore::Device->BeginCapture(Application::ExecutableDirectory / "GPU 0.wpix");
	}*/

	Device->OnBeginFrame();

	if (ImGui::Begin("GPU Timing"))
	{
		for (const auto& iter : Device->GetDevice()->GetProfiler()->Data)
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
	}
	ImGui::End();

	RHI::D3D12CommandContext& Context = Device->GetDevice()->GetCommandContext();
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
			this->Render(World, Context);

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
		InspectorWindow.SetContext(World, WorldWindow.GetSelectedActor());
		InspectorWindow.Render();

		auto [pRenderTarget, RenderTargetView] = SwapChain.GetCurrentBackBufferResource();

		auto Barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			pRenderTarget->GetResource(),
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
			pRenderTarget->GetResource(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT);
		Context->ResourceBarrier(1, &Barrier);
	}
	Context.Close();

	RendererPresent Present(Context);
	SwapChain.Present(true, Present);

	Device->OnEndFrame();
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

void Renderer::OnMove(std::int32_t x, std::int32_t y)
{
	SwapChain.DisplayHDRSupport();
}

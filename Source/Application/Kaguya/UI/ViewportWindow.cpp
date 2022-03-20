#include "ViewportWindow.h"
#include "Core/Application.h"
#include "../Renderer.h"

void ViewportWindow::OnRender()
{
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
	WorldRenderView->View.Width			 = std::clamp<uint32_t>(static_cast<int>(ViewportSize.x), 1, 3840);
	WorldRenderView->View.Height		 = std::clamp<uint32_t>(static_cast<int>(ViewportSize.y), 1, 2160);
	WorldRenderView->Camera->AspectRatio = static_cast<float>(WorldRenderView->View.Width) / static_cast<float>(WorldRenderView->View.Height);

	Renderer->OnRender(*Context, World, WorldRenderView);

	assert(Renderer->GetViewportPtr());
	ImGui::Image(Renderer->GetViewportPtr(), ViewportSize);

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

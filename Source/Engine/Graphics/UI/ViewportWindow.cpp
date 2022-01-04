#include "ViewportWindow.h"

std::pair<float, float> ViewportWindow::GetMousePosition() const
{
	auto [mx, my] = ImGui::GetMousePos();
	mx -= Rect.left;
	my -= Rect.top;

	return { mx, my };
}

void ViewportWindow::OnRender()
{
	ImGuizmo::SetDrawlist();
	ImGuizmo::SetRect(
		ImGui::GetWindowPos().x,
		ImGui::GetWindowPos().y,
		ImGui::GetWindowWidth(),
		ImGui::GetWindowHeight());

	auto viewportOffset = ImGui::GetCursorPos(); // includes tab bar
	auto viewportPos	= ImGui::GetWindowPos();
	auto viewportSize	= ImGui::GetContentRegionAvail();
	auto windowSize		= ImGui::GetWindowSize();

	Rect.left	= static_cast<LONG>(viewportPos.x + viewportOffset.x);
	Rect.top	= static_cast<LONG>(viewportPos.y + viewportOffset.y);
	Rect.right	= static_cast<LONG>(Rect.left + windowSize.x);
	Rect.bottom = static_cast<LONG>(Rect.top + windowSize.y);

	Resolution = Vec2i(static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y));

	if (pImage)
	{
		ImGui::Image(pImage, viewportSize);
	}

	if (ImGui::IsMouseDown(ImGuiMouseButton_Right) && IsHovered)
	{
		Application::InputManager.DisableCursor(pWindow->GetWindowHandle());
	}
	else if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
	{
		Application::InputManager.EnableCursor();
	}

	// Camera operates in raw input mode
	if (!Application::InputManager.CursorEnabled)
	{
		Application::InputManager.RawInputEnabled = true;
		pWindow->SetRawInput(true);
		ClipCursor(&Rect);
	}
	else
	{
		Application::InputManager.RawInputEnabled = false;
		pWindow->SetRawInput(false);
		ClipCursor(nullptr);
	}
}

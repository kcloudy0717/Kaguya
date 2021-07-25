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
	float w = (float)ImGui::GetWindowWidth();
	float h = (float)ImGui::GetWindowHeight();
	ImGuizmo::SetDrawlist();
	ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, w, h);

	auto viewportOffset = ImGui::GetCursorPos(); // includes tab bar
	auto viewportPos	= ImGui::GetWindowPos();
	auto viewportSize	= ImGui::GetContentRegionAvail();
	auto windowSize		= ImGui::GetWindowSize();

	Rect.left	= static_cast<LONG>(viewportPos.x + viewportOffset.x);
	Rect.top	= static_cast<LONG>(viewportPos.y + viewportOffset.y);
	Rect.right	= static_cast<LONG>(Rect.left + windowSize.x);
	Rect.bottom = static_cast<LONG>(Rect.top + windowSize.y);

	Resolution = Vector2i(static_cast<int>(viewportSize.x), static_cast<int>(viewportSize.y));

	if (pImage)
	{
		ImGui::Image((ImTextureID)pImage, viewportSize);
	}

	if (ImGui::IsMouseDown(ImGuiMouseButton_Right) && IsHovered)
	{
		Application::GetInputHandler().RawInputEnabled = true;
	}
	else
	{
		Application::GetInputHandler().RawInputEnabled = false;
	}
}

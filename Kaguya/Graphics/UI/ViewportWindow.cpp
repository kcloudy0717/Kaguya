#include "pch.h"
#include "ViewportWindow.h"

std::pair<float, float> ViewportWindow::GetMousePosition() const
{
	auto [mx, my] = ImGui::GetMousePos();
	mx -= Rect.left;
	my -= Rect.top;

	return { mx, my };
}

void ViewportWindow::RenderGui()
{
	Resolution = {};

	if (ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse))
	{
		UIWindow::Update();

		float w = (float)ImGui::GetWindowWidth();
		float h = (float)ImGui::GetWindowHeight();
		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, w, h);

		auto viewportOffset = ImGui::GetCursorPos(); // includes tab bar
		auto viewportPos	= ImGui::GetWindowPos();
		auto viewportSize	= ImGui::GetContentRegionAvail();
		auto windowSize		= ImGui::GetWindowSize();

		Rect.left	= viewportPos.x + viewportOffset.x;
		Rect.top	= viewportPos.y + viewportOffset.y;
		Rect.right	= Rect.left + windowSize.x;
		Rect.bottom = Rect.top + windowSize.y;

		Resolution = Vector2i(viewportSize.x, viewportSize.y);

		ImGui::Image((ImTextureID)m_pImage, viewportSize);

		if (ImGui::IsMouseDown(ImGuiMouseButton_Right) && IsHovered)
		{
			Application::GetInputHandler().RawInputEnabled = true;
		}
		else
		{
			Application::GetInputHandler().RawInputEnabled = false;
		}
	}
	ImGui::End();
}

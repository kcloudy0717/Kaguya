#include "UIWindow.h"

void UIWindow::Render()
{
	ImGui::Begin(Name.data(), nullptr, Flags);
	IsHovered = ImGui::IsWindowHovered();

	OnRender();

	ImGui::End();
}

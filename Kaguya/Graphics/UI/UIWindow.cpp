#include "pch.h"
#include "UIWindow.h"

void UIWindow::Update()
{
	IsHovered = ImGui::IsWindowHovered();
}
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
	
}

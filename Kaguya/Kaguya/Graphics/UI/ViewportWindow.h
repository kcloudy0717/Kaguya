#pragma once
#include "UIWindow.h"

class ViewportWindow : public UIWindow
{
public:
	ViewportWindow()
		: UIWindow("Viewport", ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse)
	{
	}

	void SetContext(void* pImage) { this->pImage = pImage; }

	std::pair<float, float> GetMousePosition() const;

protected:
	void OnRender() override;

public:
	Vector2i Resolution = {};
	RECT	 Rect		= {};

private:
	void* pImage = nullptr;
};

#pragma once
#include "UIWindow.h"

class ViewportWindow : public UIWindow
{
public:
	ViewportWindow()
		: UIWindow("Viewport", ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse)
	{
	}

	void SetContext(ImTextureID pImage, Window* pWindow)
	{
		this->pImage  = pImage;
		this->pWindow = pWindow;
	}

	std::pair<float, float> GetMousePosition() const;

protected:
	void OnRender() override;

public:
	RECT  Rect		 = {};

private:
	ImTextureID pImage	= nullptr;
	Window*		pWindow = nullptr;
};

#pragma once
#include "UIWindow.h"

class ViewportWindow : public UIWindow
{
public:
	void SetContext(void* pImage) { m_pImage = pImage; }

	std::pair<float, float> GetMousePosition() const;

	void RenderGui();

public:
	Vector2i Resolution = {};
	RECT	 Rect		= {};

private:
	void* m_pImage = nullptr;
};

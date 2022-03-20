#pragma once
#include "UIWindow.h"

class Renderer;
class World;
struct WorldRenderView;
class Window;
namespace RHI
{
	class D3D12CommandContext;
}

class ViewportWindow : public UIWindow
{
public:
	ViewportWindow()
		: UIWindow("Viewport", ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse)
	{
	}

	Renderer*				  Renderer		  = nullptr;
	World*					  World			  = nullptr;
	WorldRenderView*		  WorldRenderView = nullptr;
	Window*					  MainWindow	  = nullptr;
	RHI::D3D12CommandContext* Context		  = nullptr;

protected:
	void OnRender() override;
};

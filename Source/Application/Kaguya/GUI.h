#pragma once
#include <imgui.h>
#include <ImGuizmo.h>
#include "External/IconsFontAwesome5.h"

#include <RHI/RHI.h>

class GUI
{
public:
	GUI();
	~GUI();

	void Initialize(HWND HWnd, RHI::D3D12Device* Device);

	void Reset();
	void Render(RHI::D3D12CommandContext& Context);

private:
	bool Win32Initialized = false;
	bool D3D12Initialized = false;
};

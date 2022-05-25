#include "GUI.h"
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx12.h>

GUI::GUI()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	std::string IniFile = (Process::ExecutableDirectory / "imgui.ini").string();
	ImGui::LoadIniSettingsFromDisk(IniFile.data());

	ImFontConfig IconsConfig;
	IconsConfig.OversampleH = 2;
	IconsConfig.OversampleV = 2;
	ImGui::GetIO().Fonts->AddFontFromFileTTF("Resources/Fonts/CascadiaMono.ttf", 15.0f, &IconsConfig);

	// merge in icons from Font Awesome
	static constexpr ImWchar IconsRanges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
	IconsConfig.MergeMode				   = true;
	IconsConfig.PixelSnapH				   = true;
	IconsConfig.GlyphOffset				   = { 0.0f, 1.0f };
	IconsConfig.GlyphMinAdvanceX		   = 15.0f;
	ImGui::GetIO().Fonts->AddFontFromFileTTF("Resources/Fonts/" FONT_ICON_FILE_NAME_FAR, 15.0f, &IconsConfig, IconsRanges);
	// use FONT_ICON_FILE_NAME_FAR if you want regular instead of solid

	constexpr auto ColorFromBytes = [](uint8_t r, uint8_t g, uint8_t b)
	{
		return ImVec4(static_cast<float>(r) / 255.0f, static_cast<float>(g) / 255.0f, static_cast<float>(b) / 255.0f, 1.0f);
	};

	ImVec4* Colors = ImGui::GetStyle().Colors;

	const ImVec4 BgColor		  = ColorFromBytes(37, 37, 38);
	const ImVec4 LightBgColor	  = ColorFromBytes(82, 82, 85);
	const ImVec4 VeryLightBgColor = ColorFromBytes(90, 90, 95);

	const ImVec4 PanelColor		  = ColorFromBytes(51, 51, 55);
	const ImVec4 PanelHoverColor  = ColorFromBytes(29, 151, 236);
	const ImVec4 PanelActiveColor = ColorFromBytes(0, 119, 200);

	const ImVec4 TextColor		   = ColorFromBytes(255, 255, 255);
	const ImVec4 TextDisabledColor = ColorFromBytes(151, 151, 151);
	const ImVec4 BorderColor	   = ColorFromBytes(78, 78, 78);

	Colors[ImGuiCol_Text]				  = TextColor;
	Colors[ImGuiCol_TextDisabled]		  = TextDisabledColor;
	Colors[ImGuiCol_TextSelectedBg]		  = PanelActiveColor;
	Colors[ImGuiCol_WindowBg]			  = BgColor;
	Colors[ImGuiCol_ChildBg]			  = BgColor;
	Colors[ImGuiCol_PopupBg]			  = BgColor;
	Colors[ImGuiCol_Border]				  = BorderColor;
	Colors[ImGuiCol_BorderShadow]		  = BorderColor;
	Colors[ImGuiCol_FrameBg]			  = PanelColor;
	Colors[ImGuiCol_FrameBgHovered]		  = PanelHoverColor;
	Colors[ImGuiCol_FrameBgActive]		  = PanelActiveColor;
	Colors[ImGuiCol_TitleBg]			  = BgColor;
	Colors[ImGuiCol_TitleBgActive]		  = BgColor;
	Colors[ImGuiCol_TitleBgCollapsed]	  = BgColor;
	Colors[ImGuiCol_MenuBarBg]			  = PanelColor;
	Colors[ImGuiCol_ScrollbarBg]		  = PanelColor;
	Colors[ImGuiCol_ScrollbarGrab]		  = LightBgColor;
	Colors[ImGuiCol_ScrollbarGrabHovered] = VeryLightBgColor;
	Colors[ImGuiCol_ScrollbarGrabActive]  = VeryLightBgColor;
	Colors[ImGuiCol_CheckMark]			  = PanelActiveColor;
	Colors[ImGuiCol_SliderGrab]			  = PanelHoverColor;
	Colors[ImGuiCol_SliderGrabActive]	  = PanelActiveColor;
	Colors[ImGuiCol_Button]				  = PanelColor;
	Colors[ImGuiCol_ButtonHovered]		  = PanelHoverColor;
	Colors[ImGuiCol_ButtonActive]		  = PanelHoverColor;
	Colors[ImGuiCol_Header]				  = PanelColor;
	Colors[ImGuiCol_HeaderHovered]		  = PanelHoverColor;
	Colors[ImGuiCol_HeaderActive]		  = PanelActiveColor;
	Colors[ImGuiCol_Separator]			  = BorderColor;
	Colors[ImGuiCol_SeparatorHovered]	  = BorderColor;
	Colors[ImGuiCol_SeparatorActive]	  = BorderColor;
	Colors[ImGuiCol_ResizeGrip]			  = BgColor;
	Colors[ImGuiCol_ResizeGripHovered]	  = PanelColor;
	Colors[ImGuiCol_ResizeGripActive]	  = LightBgColor;
	Colors[ImGuiCol_PlotLines]			  = PanelActiveColor;
	Colors[ImGuiCol_PlotLinesHovered]	  = PanelHoverColor;
	Colors[ImGuiCol_PlotHistogram]		  = PanelActiveColor;
	Colors[ImGuiCol_PlotHistogramHovered] = PanelHoverColor;
	Colors[ImGuiCol_DragDropTarget]		  = BgColor;
	Colors[ImGuiCol_NavHighlight]		  = BgColor;
	Colors[ImGuiCol_DockingPreview]		  = PanelActiveColor;
	Colors[ImGuiCol_Tab]				  = BgColor;
	Colors[ImGuiCol_TabActive]			  = PanelActiveColor;
	Colors[ImGuiCol_TabUnfocused]		  = BgColor;
	Colors[ImGuiCol_TabUnfocusedActive]	  = PanelActiveColor;
	Colors[ImGuiCol_TabHovered]			  = PanelHoverColor;

	ImGui::GetStyle().WindowRounding	= 0.0f;
	ImGui::GetStyle().ChildRounding		= 0.0f;
	ImGui::GetStyle().FrameRounding		= 0.0f;
	ImGui::GetStyle().GrabRounding		= 0.0f;
	ImGui::GetStyle().PopupRounding		= 0.0f;
	ImGui::GetStyle().ScrollbarRounding = 0.0f;
	ImGui::GetStyle().TabRounding		= 0.0f;
}

GUI::~GUI()
{
	if (D3D12Initialized)
	{
		ImGui_ImplDX12_Shutdown();
	}
	if (Win32Initialized)
	{
		ImGui_ImplWin32_Shutdown();
	}
	ImGui::DestroyContext();
}

void GUI::Initialize(HWND HWnd, RHI::D3D12Device* Device)
{
	// Initialize ImGui for win32
	Win32Initialized = ImGui_ImplWin32_Init(HWnd);

	// Initialize ImGui for D3D12
	UINT						ImGuiIndex		   = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE ImGuiCpuDescriptor = {};
	D3D12_GPU_DESCRIPTOR_HANDLE ImGuiGpuDescriptor = {};
	Device->GetLinkedDevice()->GetResourceDescriptorHeap().Allocate(ImGuiCpuDescriptor, ImGuiGpuDescriptor, ImGuiIndex);
	D3D12Initialized = ImGui_ImplDX12_Init(Device->GetD3D12Device(), 1, RHI::D3D12SwapChain::Format, Device->GetLinkedDevice()->GetResourceDescriptorHeap(), ImGuiCpuDescriptor, ImGuiGpuDescriptor);
}

void GUI::Reset()
{
	if (Win32Initialized && D3D12Initialized)
	{
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		ImGui::DockSpaceOverViewport();
		ImGui::ShowDemoWindow();
		ImGuizmo::BeginFrame();
		ImGuizmo::AllowAxisFlip(false);
	}
}

void GUI::Render(RHI::D3D12CommandContext& Context)
{
	if (Win32Initialized && D3D12Initialized)
	{
		D3D12ScopedEvent(Context, "Gui Render");
		ImGui::Render();
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), Context.GetGraphicsCommandList());
	}
}

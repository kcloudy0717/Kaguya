// main.cpp : Defines the entry point for the application.
#if defined(_DEBUG)
// memory leak
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#define ENABLE_LEAK_DETECTION() _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF)
#define SET_LEAK_BREAKPOINT(x)	_CrtSetBreakAlloc(x)
#else
#define ENABLE_LEAK_DETECTION()
#define SET_LEAK_BREAKPOINT(x)
#endif

#include "RHI/RHI.h"

#include "Core/Application.h"
#include "Core/IApplicationMessageHandler.h"
#include "Core/Asset/AssetManager.h"
#include "Core/World/World.h"
#include "Core/World/Scripts/Player.script.h"

#include "Renderer.h"
#include "DeferredRenderer.h"
#include "PathIntegratorDXR1_0.h"
#include "PathIntegratorDXR1_1.h"

// File loader
#include "MitsubaLoader.h"

#include "Globals.h"

class ImGuiContextManager
{
public:
	ImGuiContextManager()
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::StyleColorsDark();
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

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

	~ImGuiContextManager()
	{
		if (Win32Initialized)
		{
			ImGui_ImplWin32_Shutdown();
		}
		ImGui::DestroyContext();
	}

	void InitializeWin32(HWND HWnd)
	{
		// Initialize ImGui for win32
		Win32Initialized = ImGui_ImplWin32_Init(HWnd);
	}

private:
	bool Win32Initialized = false;
};

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void ImguiMessageCallback(void* Context, HWND HWnd, UINT Message, WPARAM WParam, LPARAM LParam)
{
	UNREFERENCED_PARAMETER(Context);
	ImGui_ImplWin32_WndProcHandler(HWnd, Message, WParam, LParam);
}

class Editor final
	: public Application
	, public IApplicationMessageHandler
{
public:
	explicit Editor()
		: Application()
	{
		SetMessageHandler(this);
		RegisterMessageCallback(ImguiMessageCallback, nullptr);
	}

	bool Initialize() override
	{
		//WorldArchive::Load(ExecutableDirectory / "Assets/Scenes/cornellbox.json", World);

		std::string IniFile = (Application::ExecutableDirectory / "imgui.ini").string();
		ImGui::LoadIniSettingsFromDisk(IniFile.data());

		Renderer->OnInitialize();

		return true;
	}

	void Shutdown() override
	{
		Renderer->OnDestroy();
	}

	void Update() override
	{
		Stopwatch.Signal();
		DeltaTime = static_cast<float>(Stopwatch.GetDeltaTime());
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();

		ImGui::DockSpaceOverViewport();

		ImGui::ShowDemoWindow();

		ImGuizmo::AllowAxisFlip(false);

		World->Update(DeltaTime);
		Renderer->OnRender(World);
	}

	void OnKeyDown(unsigned char KeyCode, bool IsRepeat) override
	{
	}

	void OnKeyUp(unsigned char KeyCode) override
	{
	}

	void OnKeyChar(unsigned char Character, bool IsRepeat) override
	{
	}

	void OnMouseMove(int X, int Y) override
	{
	}

	void OnMouseDown(const Window* Window, EMouseButton Button, int X, int Y) override
	{
	}

	void OnMouseUp(EMouseButton Button, int X, int Y) override
	{
	}

	void OnMouseDoubleClick(const Window* Window, EMouseButton Button, int X, int Y) override
	{
	}

	void OnMouseWheel(float Delta, int X, int Y) override
	{
	}

	void OnRawMouseMove(int X, int Y) override
	{
		if (World)
		{
			Actor MainCamera = World->GetMainCamera();
			auto& Camera	 = MainCamera.GetComponent<CameraComponent>();

			if (MainWindow->IsUsingRawInput())
			{
				Camera.Rotate(
					Y * DeltaTime * Camera.MouseSensitivityY,
					X * DeltaTime * Camera.MouseSensitivityX,
					0.0f);
			}
		}
	}

	void OnWindowClose(Window* Window) override
	{
		Window->Destroy();
		if (Window == MainWindow)
		{
			RequestExit = true;
		}
	}

	void OnWindowResize(Window* Window, std::int32_t Width, std::int32_t Height) override
	{
		Window->Resize(Width, Height);
		if (Window == MainWindow)
		{
			if (Renderer)
			{
				Renderer->OnResize(Width, Height);
			}
		}
	}

	void OnWindowMove(Window* Window, std::int32_t x, std::int32_t y) override
	{
		if (Window == MainWindow)
		{
			if (Renderer)
			{
				Renderer->OnMove(x, y);
			}
		}
	}

	Stopwatch Stopwatch;
	Window*	  MainWindow = nullptr;

	World*	  World	   = nullptr;
	Renderer* Renderer = nullptr;

	float DeltaTime;
};

int main(int /*argc*/, char* /*argv*/[])
{
	ENABLE_LEAK_DETECTION();
	SET_LEAK_BREAKPOINT(-1);

	Editor				Editor;
	ImGuiContextManager ImGui;

	Window MainWindow;

	{
		WINDOW_DESC WindowDesc = {};
		WindowDesc.Name		   = L"Kaguya";
		WindowDesc.Width	   = 1280;
		WindowDesc.Height	   = 720;
		WindowDesc.InitialSize = WindowInitialSize::Maximize;
		Editor.AddWindow(nullptr, &MainWindow, WindowDesc);
	}
	ImGui.InitializeWin32(MainWindow.GetWindowHandle());

	MainWindow.Show();

	RHI::DeviceOptions DeviceOptions = {};
#if _DEBUG
	DeviceOptions.EnableDebugLayer		   = true;
	DeviceOptions.EnableGpuBasedValidation = false;
	DeviceOptions.EnableAutoDebugName	   = true;
#endif
	DeviceOptions.FeatureLevel	   = D3D_FEATURE_LEVEL_12_0;
	DeviceOptions.Raytracing	   = true;
	DeviceOptions.DynamicResources = true;
	DeviceOptions.MeshShaders	   = true;
	DeviceOptions.PsoCachePath	   = Application::ExecutableDirectory / "Pso.cache";
	D3D12RHIInitializer		D3D12RHIInitializer(DeviceOptions);
	AssetManagerInitializer AssetManagerInitializer;

	World World(Kaguya::AssetManager);
	World.ActiveCameraActor.AddComponent<NativeScriptComponent>().Bind<PlayerScript>();

	//MitsubaLoader::Load(Application::ExecutableDirectory / "Assets/Models/coffee/scene.xml", Kaguya::AssetManager, &World);
	//MitsubaLoader::Load(Application::ExecutableDirectory / "Assets/Models/bathroom/scene.xml", Kaguya::AssetManager, &World);
	//MitsubaLoader::Load(Application::ExecutableDirectory / "Assets/Models/staircase2/scene.xml", Kaguya::AssetManager, &World);

	//DeferredRenderer Renderer(Kaguya::Device, Kaguya::Compiler, &MainWindow);
	//PathIntegratorDXR1_0 Renderer(Kaguya::Device, Kaguya::Compiler, &MainWindow);
	PathIntegratorDXR1_1 Renderer(Kaguya::Device, Kaguya::Compiler, &MainWindow);

	Editor.MainWindow = &MainWindow;
	Editor.World	  = &World;
	Editor.Renderer	  = &Renderer;
	Editor.Run();
}

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

#include "Core/Application.h"
#include "Core/IApplicationMessageHandler.h"
#include "Core/Asset/AssetManager.h"
#include "RenderCore/RenderCore.h"

#include "Graphics/UI/WorldWindow.h"
#include "Graphics/UI/ViewportWindow.h"
#include "Graphics/UI/InspectorWindow.h"
#include "Graphics/UI/AssetWindow.h"
#include "Graphics/UI/ConsoleWindow.h"

#include "Graphics/Renderer.h"
#include "Graphics/PathIntegratorDXR1_0.h"
#include "Graphics/PathIntegratorDXR1_1.h"
#include "World/WorldArchive.h"

class ImGuiContextManager
{
public:
	ImGuiContextManager()
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::StyleColorsDark();
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
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

class Editor final
	: public Application
	, public IApplicationMessageHandler
{
public:
	explicit Editor(const std::string& LoggerName, const ApplicationOptions& Options)
		: Application(LoggerName, Options)
	{
		SetMessageHandler(this);
	}

	bool Initialize() override
	{
		AssetManager::Initialize();

		//WorldArchive::Load(ExecutableDirectory / "Assets/Scenes/cornellbox.json", World);

		std::string IniFile = (Application::ExecutableDirectory / "imgui.ini").string();
		ImGui::LoadIniSettingsFromDisk(IniFile.data());

		WorldWindow.SetContext(World);
		InspectorWindow.SetContext(World, {});
		AssetWindow.SetContext(World);

		Renderer->OnInitialize();

		return true;
	}

	void Shutdown() override
	{
		Renderer->OnDestroy();

		AssetManager::Shutdown();
	}

	void Update(float DeltaTime) override
	{
		World->WorldState = EWorldState::EWorldState_Render;

		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();

		ImGui::DockSpaceOverViewport();

		ImGui::ShowDemoWindow();

		ImGuizmo::AllowAxisFlip(false);

		WorldWindow.Render();
		InspectorWindow.SetContext(World, WorldWindow.GetSelectedActor());

		ViewportWindow.SetContext(Renderer->GetViewportDescriptor(), MainWindow);
		ViewportWindow.Render();

		AssetWindow.Render();
		ConsoleWindow.Render();
		InspectorWindow.Render();

		const uint32_t ViewportWidth = ViewportWindow.Resolution.x, ViewportHeight = ViewportWindow.Resolution.y;
		// const uint32_t ViewportWidth = 3840, ViewportHeight = 2160;
		// const uint32_t ViewportWidth = 1920, ViewportHeight = 1080;

		World->Update(DeltaTime);

		// Render
		Renderer->OnSetViewportResolution(ViewportWidth, ViewportHeight);

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

	Window* MainWindow = nullptr;

	World*	  World	   = nullptr;
	Renderer* Renderer = nullptr;

	WorldWindow		WorldWindow;
	ViewportWindow	ViewportWindow;
	InspectorWindow InspectorWindow;
	AssetWindow		AssetWindow;
	ConsoleWindow	ConsoleWindow;
};

int main(int /*argc*/, char* /*argv*/[])
{
	ENABLE_LEAK_DETECTION();
	SET_LEAK_BREAKPOINT(-1);

	ApplicationOptions Options = {};
	Options.Icon			   = Application::ExecutableDirectory / "Assets/Kaguya.ico";

	Editor				Editor("Editor", Options);
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

	DeviceOptions DeviceOptions = {};
#if _DEBUG
	DeviceOptions.EnableDebugLayer		   = true;
	DeviceOptions.EnableGpuBasedValidation = false;
	DeviceOptions.EnableAutoDebugName	   = true;
#endif
	DeviceOptions.FeatureLevel	   = D3D_FEATURE_LEVEL_12_0;
	DeviceOptions.Raytracing	   = true;
	DeviceOptions.DynamicResources = true;
	DeviceOptions.MeshShaders	   = true;
	RenderCoreInitializer Render(DeviceOptions);

	World World;

	// PathIntegratorDXR1_0 Renderer(MainWindow.GetWindowHandle());
	PathIntegratorDXR1_1 Renderer(MainWindow.GetWindowHandle());
	// DeferredRenderer Renderer(MainWindow.GetWindowHandle());

	Editor.MainWindow = &MainWindow;
	Editor.World	  = &World;
	Editor.Renderer	  = &Renderer;
	Editor.Run();
}

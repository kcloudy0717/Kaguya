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
#include "Graphics/PathIntegrator.h"
#include "Graphics/DeferredRenderer.h"

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
		InspectorWindow.SetContext(World, WorldWindow.GetSelectedEntity());

		ViewportWindow.SetContext(Renderer->GetViewportDescriptor(), MainWindow);
		ViewportWindow.Render();

		AssetWindow.Render();
		ConsoleWindow.Render();
		InspectorWindow.Render();

		const uint32_t ViewportWidth = ViewportWindow.Resolution.x, ViewportHeight = ViewportWindow.Resolution.y;

		World->Update(DeltaTime);

		// Render
		Renderer->OnSetViewportResolution(ViewportWidth, ViewportHeight);

		Renderer->OnRender(World);
	}

	void OnKeyDown(unsigned char KeyCode, bool IsRepeat) override
	{
		IApplicationMessageHandler::OnKeyDown(KeyCode, IsRepeat);
	}

	void OnKeyUp(unsigned char KeyCode) override { IApplicationMessageHandler::OnKeyUp(KeyCode); }

	void OnKeyChar(unsigned char Character, bool IsRepeat) override
	{
		IApplicationMessageHandler::OnKeyChar(Character, IsRepeat);
	}

	void OnMouseMove(Vector2i Position) override { IApplicationMessageHandler::OnMouseMove(Position); }

	void OnMouseDown(const Window* Window, EMouseButton Button, Vector2i Position) override
	{
		IApplicationMessageHandler::OnMouseDown(Window, Button, Position);
	}

	void OnMouseUp(EMouseButton Button, Vector2i Position) override
	{
		IApplicationMessageHandler::OnMouseUp(Button, Position);
	}

	void OnMouseDoubleClick(const Window* Window, EMouseButton Button, Vector2i Position) override
	{
		IApplicationMessageHandler::OnMouseDoubleClick(Window, Button, Position);
	}

	void OnMouseWheel(float Delta, Vector2i Position) override
	{
		IApplicationMessageHandler::OnMouseWheel(Delta, Position);
	}

	void OnRawMouseMove(Vector2i Position) override
	{
		IApplicationMessageHandler::OnRawMouseMove(Position);

		if (World)
		{
			Entity MainCamera = World->GetMainCamera();
			auto&  Camera	  = MainCamera.GetComponent<CameraComponent>();

			if (MainWindow->IsUsingRawInput())
			{
				Camera.Rotate(
					Position.y * DeltaTime * Camera.MouseSensitivityY,
					Position.x * DeltaTime * Camera.MouseSensitivityX,
					0.0f);
			}
		}
	}

	void OnWindowClose(Window* Window) override
	{
		IApplicationMessageHandler::OnWindowClose(Window);

		Window->Destroy();
		if (Window == MainWindow)
		{
			RequestExit = true;
		}
	}

	void OnWindowResize(Window* Window, std::int32_t Width, std::int32_t Height) override
	{
		IApplicationMessageHandler::OnWindowResize(Window, Width, Height);

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
		WindowDesc.Maximize	   = true;
		Editor.AddWindow(nullptr, &MainWindow, WindowDesc);
	}
	ImGui.InitializeWin32(MainWindow.GetWindowHandle());

	MainWindow.Show();

	DeviceOptions DeviceOptions			   = {};
	DeviceOptions.EnableDebugLayer		   = true;
	DeviceOptions.EnableGpuBasedValidation = false;
	DeviceOptions.EnableAutoDebugName	   = true;
	DeviceFeatures DeviceFeatures		   = {};
	DeviceFeatures.FeatureLevel			   = D3D_FEATURE_LEVEL_12_0;
	DeviceFeatures.Raytracing			   = true;
	DeviceFeatures.DynamicResources		   = true;
	DeviceFeatures.MeshShaders			   = true;
	RenderCoreInitializer Render(DeviceOptions, DeviceFeatures);

	World World;
	// PathIntegrator Renderer(MainWindow.GetWindowHandle());
	DeferredRenderer Renderer(MainWindow.GetWindowHandle());

	Editor.MainWindow = &MainWindow;
	Editor.World	  = &World;
	Editor.Renderer	  = &Renderer;
	Editor.Run();
}

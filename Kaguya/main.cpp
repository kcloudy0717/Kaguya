// main.cpp : Defines the entry point for the application.
#if defined(_DEBUG)
// memory leak
#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>
#define ENABLE_LEAK_DETECTION() _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF)
#define SET_LEAK_BREAKPOINT(x)	_CrtSetBreakAlloc(x)
#else
#define ENABLE_LEAK_DETECTION() 0
#define SET_LEAK_BREAKPOINT(X)	X
#endif

#include <Core/Application.h>
#include <Core/Asset/AssetManager.h>
#include <Physics/PhysicsManager.h>
#include <RenderCore/RenderCore.h>

#include <Graphics/UI/HierarchyWindow.h>
#include <Graphics/UI/ViewportWindow.h>
#include <Graphics/UI/InspectorWindow.h>
#include <Graphics/UI/AssetWindow.h>
#include <Graphics/UI/ConsoleWindow.h>

#include <Graphics/Renderer.h>
#include <Graphics/PathIntegrator.h>
#include <Graphics/DeferredRenderer.h>

#define RENDER_AT_1920x1080 0

class Editor final : public Application
{
public:
	Editor()
		: Application("Editor")
	{
	}

	~Editor() override {}

	bool Initialize() override
	{
		DeviceOptions  DeviceOptions	= { .EnableDebugLayer		  = true,
										.EnableGpuBasedValidation = false,
										.EnableAutoDebugName	  = true };
		DeviceFeatures DeviceFeatures	= {};
		DeviceFeatures.FeatureLevel		= D3D_FEATURE_LEVEL_12_0;
		DeviceFeatures.Raytracing		= true;
		DeviceFeatures.DynamicResources = true;
		DeviceFeatures.MeshShaders		= true;

		PhysicsManager::Initialize();
		RenderCore::Initialize(DeviceOptions, DeviceFeatures);
		AssetManager::Initialize();

		std::string IniFile = (ExecutableDirectory / "imgui.ini").string();
		ImGui::LoadIniSettingsFromDisk(IniFile.data());

		HierarchyWindow.SetContext(&World);
		InspectorWindow.SetContext(&World, {});
		AssetWindow.SetContext(&World);

		// Renderer = std::make_unique<PathIntegrator>();
		Renderer = std::make_unique<DeferredRenderer>();
		Renderer->OnInitialize();

		return true;
	}

	void Update(float DeltaTime) override
	{
		World.WorldState = EWorldState::EWorldState_Render;

		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();

		ImGui::DockSpaceOverViewport();

		ImGui::ShowDemoWindow();

		ImGuizmo::AllowAxisFlip(false);

		HierarchyWindow.Render();
		InspectorWindow.SetContext(&World, HierarchyWindow.GetSelectedEntity());

		ViewportWindow.Render();
		ViewportWindow.SetContext(Renderer->GetViewportDescriptor());

		AssetWindow.Render();
		ConsoleWindow.Render();
		InspectorWindow.Render();

#if RENDER_AT_1920x1080
		const uint32_t ViewportWidth = 1920, ViewportHeight = 1080;
#else
		const uint32_t ViewportWidth = ViewportWindow.Resolution.x, ViewportHeight = ViewportWindow.Resolution.y;
#endif

		World.Update(DeltaTime);

		// Render
		Renderer->OnSetViewportResolution(ViewportWidth, ViewportHeight);

		Renderer->OnRender(&World);
	}

	void Shutdown() override
	{
		Renderer->OnDestroy();
		Renderer.reset();
		World.Clear();
		AssetManager::Shutdown();
		RenderCore::Shutdown();
		PhysicsManager::Shutdown();
	}

	void Resize(UINT Width, UINT Height) override { Renderer->OnResize(Width, Height); }

private:
	HierarchyWindow HierarchyWindow;
	ViewportWindow	ViewportWindow;
	InspectorWindow InspectorWindow;
	AssetWindow		AssetWindow;
	ConsoleWindow	ConsoleWindow;

	World					  World;
	std::unique_ptr<Renderer> Renderer;
};

int main(int /*argc*/, char* /*argv*/[])
{
#if defined(_DEBUG)
	ENABLE_LEAK_DETECTION();
	SET_LEAK_BREAKPOINT(-1);
#endif

	try
	{
		ApplicationOptions Options = {};
		Options.Name			   = L"Kaguya";
		Options.Width			   = 1280;
		Options.Height			   = 720;
		Options.Maximize		   = true;
		Options.Icon			   = Application::ExecutableDirectory / "Assets/Kaguya.ico";

		Editor App;
		Application::Run(App, Options);
	}
	catch (std::exception& Exception)
	{
		MessageBoxA(nullptr, Exception.what(), "Error", MB_OK | MB_ICONERROR | MB_DEFAULT_DESKTOP_ONLY);
		return 1;
	}

	return 0;
}

// main.cpp : Defines the entry point for the application.
#if defined(_DEBUG)
// memory leak
#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>
#define ENABLE_LEAK_DETECTION() _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF)
#define SET_LEAK_BREAKPOINT(x)	_CrtSetBreakAlloc(x)
#else
#define ENABLE_LEAK_DETECTION()
#define SET_LEAK_BREAKPOINT(x)
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

int main(int /*argc*/, char* /*argv*/[])
{
#if defined(_DEBUG)
	ENABLE_LEAK_DETECTION();
	SET_LEAK_BREAKPOINT(-1);
#endif

	try
	{
		Application Application("Editor");

		HierarchyWindow HierarchyWindow;
		ViewportWindow	ViewportWindow;
		InspectorWindow InspectorWindow;
		AssetWindow		AssetWindow;
		ConsoleWindow	ConsoleWindow;

		World					  World;
		std::unique_ptr<Renderer> Renderer;

		Application.Initialize = [&]()
		{
			DeviceOptions Options			 = {};
			Options.EnableDebugLayer		 = true;
			Options.EnableGpuBasedValidation = false;
			Options.EnableAutoDebugName		 = true;
			DeviceFeatures Features			 = {};
			Features.FeatureLevel			 = D3D_FEATURE_LEVEL_12_0;
			Features.Raytracing				 = true;
			Features.DynamicResources		 = true;
			Features.MeshShaders			 = true;

			PhysicsManager::Initialize();
			RenderCore::Initialize(Options, Features);
			AssetManager::Initialize();

			std::string IniFile = (Application::ExecutableDirectory / "imgui.ini").string();
			ImGui::LoadIniSettingsFromDisk(IniFile.data());

			HierarchyWindow.SetContext(&World);
			InspectorWindow.SetContext(&World, {});
			AssetWindow.SetContext(&World);

			// Renderer = std::make_unique<PathIntegrator>();
			Renderer = std::make_unique<DeferredRenderer>();
			Renderer->OnInitialize();

			return true;
		};

		Application.Update = [&](float DeltaTime)
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
		};

		Application.Shutdown = [&]()
		{
			Renderer->OnDestroy();
			Renderer.reset();
			World.Clear();
			AssetManager::Shutdown();
			RenderCore::Shutdown();
			PhysicsManager::Shutdown();
		};

		Application.Resize = [&](UINT Width, UINT Height)
		{
			Renderer->OnResize(Width, Height);
		};

		ApplicationOptions Options = {};
		Options.Name			   = L"Kaguya";
		Options.Width			   = 1280;
		Options.Height			   = 720;
		Options.Maximize		   = true;
		Options.Icon			   = Application::ExecutableDirectory / "Assets/Kaguya.ico";
		Application.Run(Options);
	}
	catch (std::exception& Exception)
	{
		MessageBoxA(nullptr, Exception.what(), "Error", MB_OK | MB_ICONERROR | MB_DEFAULT_DESKTOP_ONLY);
		return 1;
	}

	return 0;
}

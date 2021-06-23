// main.cpp : Defines the entry point for the application.
//
#include "pch.h"

#define NOMINMAX
#include <Core/Application.h>
#include <Graphics/RenderDevice.h>
#include <Graphics/AssetManager.h>
#include <Graphics/Renderer.h>
#include <Graphics/UI/HierarchyWindow.h>
#include <Graphics/UI/ViewportWindow.h>
#include <Graphics/UI/InspectorWindow.h>
#include <Graphics/UI/RenderSystemWindow.h>
#include <Graphics/UI/AssetWindow.h>
#include <Graphics/UI/ConsoleWindow.h>

#define SHOW_IMGUI_DEMO_WINDOW 1

#define RENDER_AT_1920x1080	   0

class Editor
{
public:
	Editor()
	{
		std::string iniFile = (Application::ExecutableDirectory / "imgui.ini").string();

		ImGui::LoadIniSettingsFromDisk(iniFile.data());

		m_HierarchyWindow.SetContext(&Scene);
		m_InspectorWindow.SetContext(&Scene, {});
		m_RenderSystemWindow.SetContext(&Renderer);
		m_AssetWindow.SetContext(&Scene);

		Renderer.OnInitialize();
	}

	~Editor() { Renderer.OnDestroy(); }

	void Render()
	{
		Stopwatch&	  Time		   = Application::Time;
		InputHandler& InputHandler = Application::GetInputHandler();
		Mouse&		  Mouse		   = Application::GetMouse();
		Keyboard&	  Keyboard	   = Application::GetKeyboard();

		Time.Signal();
		float dt = Time.DeltaTime();

		Scene.SceneState = Scene::SCENE_STATE_RENDER;

		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();

		ImGui::DockSpaceOverViewport();

#if SHOW_IMGUI_DEMO_WINDOW
		ImGui::ShowDemoWindow();
#endif

		ImGuizmo::AllowAxisFlip(false);

		m_ViewportWindow.SetContext((void*)Renderer.GetViewportDescriptor().GpuHandle.ptr);

		m_HierarchyWindow.RenderGui();
		m_ViewportWindow.RenderGui();
		m_RenderSystemWindow.RenderGui();
		m_AssetWindow.RenderGui();
		m_ConsoleWindow.RenderGui();

		// Update selected entity here in case Clear is called on HierarchyWindow to ensure entity is invalidated
		m_InspectorWindow.SetContext(&Scene, m_HierarchyWindow.GetSelectedEntity());
		m_InspectorWindow.RenderGui();

		auto [vpx, vpy] = m_ViewportWindow.GetMousePosition();
#if RENDER_AT_1920x1080
		// TODO: Fix: Picking will become buggy when RENDER_AT_1920x1080 is enabled
		uint32_t viewportWidth = 1920, viewportHeight = 1080;
#else
		uint32_t viewportWidth = m_ViewportWindow.Resolution.x, viewportHeight = m_ViewportWindow.Resolution.y;
#endif

		// Update selected entity
		// If LMB is pressed and we are not handling raw input and if we are not hovering over any imgui stuff then we
		// update the instance id for editor
		if (Mouse.IsLeftPressed() && !InputHandler.RawInputEnabled && m_ViewportWindow.IsHovered &&
			!ImGuizmo::IsUsing())
		{
			m_HierarchyWindow.SetSelectedEntity(Renderer.GetSelectedEntity());
		}

		Scene.Update(dt);

		// Render
		Renderer.SetViewportMousePosition(vpx, vpy);
		Renderer.SetViewportResolution(viewportWidth, viewportHeight);

		Renderer.OnRender(Time, Scene);
	}

	void Resize(uint32_t Width, uint32_t Height) { Renderer.OnResize(Width, Height); }

private:
	HierarchyWindow	   m_HierarchyWindow;
	ViewportWindow	   m_ViewportWindow;
	InspectorWindow	   m_InspectorWindow;
	RenderSystemWindow m_RenderSystemWindow;
	AssetWindow		   m_AssetWindow;
	ConsoleWindow	   m_ConsoleWindow;

	Scene	 Scene;
	Renderer Renderer;
};

int main(int argc, char* argv[])
{
	try
	{
		Application::InitializeComponents();

		ApplicationOptions options = { .Name	 = L"Kaguya",
									   .Width	 = 1280,
									   .Height	 = 720,
									   .Maximize = true,
									   .Icon	 = Application::ExecutableDirectory / "Assets/Kaguya.ico" };
		Application::Initialize(options);

		RenderDevice::Initialize();
		AssetManager::Initialize();

		std::unique_ptr<Editor> editor = std::make_unique<Editor>();

		Application::Window.SetRenderFunc(
			[&]()
			{
				editor->Render();
			});

		Application::Window.SetResizeFunc(
			[&](UINT Width, UINT Height)
			{
				editor->Resize(Width, Height);
			});

		Application::Time.Restart();
		Application::Run();

		editor.reset();
		AssetManager::Shutdown();
		RenderDevice::Shutdown();
	}
	catch (std::exception& e)
	{
		MessageBoxA(nullptr, e.what(), "Error", MB_OK | MB_ICONERROR | MB_DEFAULT_DESKTOP_ONLY);
		return 1;
	}

	return 0;
}

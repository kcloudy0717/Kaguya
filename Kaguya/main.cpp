// main.cpp : Defines the entry point for the application.
//
#include "pch.h"

#if defined(_DEBUG)
// memory leak
#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>
#define ENABLE_LEAK_DETECTION() _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF)
#define SET_LEAK_BREAKPOINT(x) _CrtSetBreakAlloc(x)
#else
#define ENABLE_LEAK_DETECTION() 0
#define SET_LEAK_BREAKPOINT(X) X
#endif

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

#define RENDER_AT_1920x1080 0

class Editor
{
public:
	Editor()
	{
		std::string iniFile = (Application::ExecutableFolderPath / "imgui.ini").string();

		ImGui::LoadIniSettingsFromDisk(iniFile.data());

		m_HierarchyWindow.SetContext(&Scene);
		m_InspectorWindow.SetContext(&Scene, {});
		m_RenderSystemWindow.SetContext(&Renderer);
		m_AssetWindow.SetContext(&Scene);

		Renderer.OnInitialize();
	}

	~Editor()
	{
		Renderer.OnDestroy();
	}

	void Render()
	{
		Time& Time = Application::Time;
		Mouse& Mouse = Application::InputHandler.Mouse;
		Keyboard& Keyboard = Application::InputHandler.Keyboard;

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

		// Update
		Scene.PreviousCamera = Scene.Camera;

		Scene.Camera.AspectRatio = static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight);

		// Update selected entity
		// If LMB is pressed and we are not handling raw input and if we are not hovering over any imgui stuff then we update the
		// instance id for editor
		if (Mouse.IsLMBPressed() && !Mouse.UseRawInput && m_ViewportWindow.IsHovered && !ImGuizmo::IsUsing())
		{
			m_HierarchyWindow.SetSelectedEntity(Renderer.GetSelectedEntity());
		}

		if (Mouse.UseRawInput)
		{
			if (Keyboard.IsKeyPressed('W'))
				Scene.Camera.Translate(0.0f, 0.0f, dt);
			if (Keyboard.IsKeyPressed('A'))
				Scene.Camera.Translate(-dt, 0.0f, 0.0f);
			if (Keyboard.IsKeyPressed('S'))
				Scene.Camera.Translate(0.0f, 0.0f, -dt);
			if (Keyboard.IsKeyPressed('D'))
				Scene.Camera.Translate(dt, 0.0f, 0.0f);
			if (Keyboard.IsKeyPressed('E'))
				Scene.Camera.Translate(0.0f, dt, 0.0f);
			if (Keyboard.IsKeyPressed('Q'))
				Scene.Camera.Translate(0.0f, -dt, 0.0f);

			while (const auto rawInput = Mouse.ReadRawInput())
			{
				Scene.Camera.Rotate(rawInput->Y * dt, rawInput->X * dt);
			}
		}

		Scene.Update();

		// Render
		Renderer.SetViewportMousePosition(vpx, vpy);
		Renderer.SetViewportResolution(viewportWidth, viewportHeight);

		Renderer.OnRender(Time, Scene);
	}

	void Resize(uint32_t Width, uint32_t Height)
	{
		Renderer.OnResize(Width, Height);
	}
private:
	HierarchyWindow	m_HierarchyWindow;
	ViewportWindow m_ViewportWindow;
	InspectorWindow m_InspectorWindow;
	RenderSystemWindow m_RenderSystemWindow;
	AssetWindow m_AssetWindow;
	ConsoleWindow m_ConsoleWindow;

	Scene Scene;
	Renderer Renderer;
};

int main(int argc, char* argv[])
{
#if defined(_DEBUG)
	ENABLE_LEAK_DETECTION();
	SET_LEAK_BREAKPOINT(-1);
#endif

	atexit(Device::ReportLiveObjects);

	Application::Config config = {
		.Title = L"Kaguya",
		.Width = 1280,
		.Height = 720,
		.Maximize = true };

	Application::Initialize(config);
	RenderDevice::Initialize();
	AssetManager::Initialize();

	std::unique_ptr<Editor> editor = std::make_unique<Editor>();

	Application::Window.SetRenderFunc([&]()
	{
		editor->Render();
	});

	Application::Window.SetResizeFunc([&](UINT Width, UINT Height)
	{
		editor->Resize(Width, Height);
	});

	Application::Time.Restart();
	return Application::Run([&]()
	{
		editor.reset();
		AssetManager::Shutdown();
		RenderDevice::Shutdown();
	});
}
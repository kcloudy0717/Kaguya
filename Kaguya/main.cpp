// main.cpp : Defines the entry point for the application.
//
#include <Core/Application.h>
#include <Physics/PhysicsManager.h>
#include <RenderCore/RenderCore.h>

#include <Graphics/AssetManager.h>
#include <Graphics/Renderer.h>
#include <Graphics/UI/HierarchyWindow.h>
#include <Graphics/UI/ViewportWindow.h>
#include <Graphics/UI/InspectorWindow.h>
#include <Graphics/UI/AssetWindow.h>
#include <Graphics/UI/ConsoleWindow.h>

#include <RenderGraph/RenderGraph.h>

#define RENDER_AT_1920x1080 0

class Editor : public Application
{
public:
	Editor() {}

	~Editor() {}

	bool Initialize() override
	{
		atexit(Adapter::ReportLiveObjects);
		PhysicsManager::Initialize();
		RenderCore::Initialize();
		AssetManager::Initialize();

		std::string iniFile = (Application::ExecutableDirectory / "imgui.ini").string();

		ImGui::LoadIniSettingsFromDisk(iniFile.data());

		HierarchyWindow.SetContext(&World);
		InspectorWindow.SetContext(&World, {});
		AssetWindow.SetContext(&World);

		pRenderer = std::make_unique<Renderer>(World);
		pRenderer->OnInitialize();

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

		ViewportWindow.SetContext((void*)pRenderer->GetViewportDescriptor().GetGPUHandle().ptr);
		// Update selected entity here in case Clear is called on HierarchyWindow to ensure entity is invalidated
		InspectorWindow.SetContext(&World, HierarchyWindow.GetSelectedEntity());

		HierarchyWindow.Render();
		ViewportWindow.Render();
		AssetWindow.Render();
		ConsoleWindow.Render();
		InspectorWindow.Render();

		auto [vpx, vpy] = ViewportWindow.GetMousePosition();
#if RENDER_AT_1920x1080
		const uint32_t viewportWidth = 1920, viewportHeight = 1080;
#else
		const uint32_t viewportWidth = ViewportWindow.Resolution.x, viewportHeight = ViewportWindow.Resolution.y;
#endif

		World.Update(DeltaTime);

		// Render
		pRenderer->SetViewportMousePosition(vpx, vpy);
		pRenderer->SetViewportResolution(viewportWidth, viewportHeight);

		pRenderer->OnRender();
	}

	void Shutdown() override
	{
		pRenderer->OnDestroy();
		pRenderer.reset();
		World.Clear();
		AssetManager::Shutdown();
		RenderCore::Shutdown();
		PhysicsManager::Shutdown();
	}

	void Resize(UINT Width, UINT Height) override { pRenderer->OnResize(Width, Height); }

private:
	HierarchyWindow HierarchyWindow;
	ViewportWindow	ViewportWindow;
	InspectorWindow InspectorWindow;
	AssetWindow		AssetWindow;
	ConsoleWindow	ConsoleWindow;

	World					  World;
	std::unique_ptr<Renderer> pRenderer;
};

int main(int argc, char* argv[])
{
	/*RenderGraph RenderGraph;

	RenderResourceHandle PathTraceOutput;
	RenderGraph.AddRenderPass(
		"Path Trace",
		[&](RenderGraphScheduler& Scheduler, RenderScope& Scope)
		{
			PathTraceOutput = Scheduler.CreateTexture(RGTextureSize::Dynamic, RGTextureDesc());

			return [](RenderGraphRegistry& Registry, CommandContext& Context)
			{

			};
		});

	struct Tonemap
	{
		RootSignature RS;
		PipelineState PSO;

		RenderTargetView   RTV;
		ShaderResourceView SRV;
	};
	RenderResourceHandle TonemapOutput;
	RenderGraph.AddRenderPass(
		"Tonemap",
		[&](RenderGraphScheduler& Scheduler, RenderScope& Scope)
		{
			auto& Parameter = Scope.Get<Tonemap>();

			TonemapOutput = Scheduler.CreateTexture(RGTextureSize::Dynamic, RGTextureDesc());

			Scheduler.Read(PathTraceOutput);

			return [](RenderGraphRegistry& Registry, CommandContext& Context)
			{

			};
		});

	RenderResourceHandle FSREASUOutput;
	RenderGraph.AddRenderPass(
		"FSR EASU",
		[&](RenderGraphScheduler& Scheduler, RenderScope& Scope)
		{
			FSREASUOutput = Scheduler.CreateTexture(RGTextureSize::Viewport, RGTextureDesc());

			Scheduler.Read(TonemapOutput);

			return [](RenderGraphRegistry& Registry, CommandContext& Context)
			{

			};
		});

	RenderResourceHandle FSRRCASOutput;
	RenderGraph.AddRenderPass(
		"FSR RCAS",
		[&](RenderGraphScheduler& Scheduler, RenderScope& Scope)
		{
			FSRRCASOutput = Scheduler.CreateTexture(RGTextureSize::Viewport, RGTextureDesc());

			Scheduler.Read(FSREASUOutput);

			return [](RenderGraphRegistry& Registry, CommandContext& Context)
			{

			};
		});

	RenderGraph.Setup();
	RenderGraph.Compile();*/

	try
	{
		Application::InitializeComponents();

		const ApplicationOptions AppOptions = { .Name	  = L"Kaguya",
												.Width	  = 1280,
												.Height	  = 720,
												.Maximize = true,
												.Icon	  = Application::ExecutableDirectory / "Assets/Kaguya.ico" };

		Editor App;
		Application::Run(App, AppOptions);
	}
	catch (std::exception& Exception)
	{
		MessageBoxA(nullptr, Exception.what(), "Error", MB_OK | MB_ICONERROR | MB_DEFAULT_DESKTOP_ONLY);
		return 1;
	}

	return 0;
}

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

#include "System/Application.h"
#include "System/IApplicationMessageHandler.h"

#include "Core/World/World.h"
#include "Core/World/WorldArchive.h"

#include "DeferredRenderer.h"
#include "PathIntegratorDXR1_0.h"
#include "PathIntegratorDXR1_1.h"

// File loader
#include "MitsubaLoader.h"

#include "Globals.h"

#include "UI/WorldWindow.h"
#include "UI/InspectorWindow.h"
#include "UI/AssetWindow.h"
#include "UI/ViewportWindow.h"

// https://devblogs.microsoft.com/directx/gettingstarted-dx12agility/
extern "C"
{
	_declspec(dllexport) extern const UINT D3D12SDKVersion = 602;
	_declspec(dllexport) extern const char* D3D12SDKPath   = ".\\D3D12\\";
}

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

enum class RENDER_PATH
{
	DeferredRenderer,
	PathIntegratorDXR1_0,
	PathIntegratorDXR1_1,
};

class EditorCamera
{
public:
	void OnUpdate(float DeltaTime)
	{
		bool Fwd = false, Bwd = false, Right = false, Left = false, Up = false, Down = false;
		if (Application::InputManager.RawInputEnabled)
		{
			Fwd	  = Application::InputManager.IsPressed('W');
			Bwd	  = Application::InputManager.IsPressed('S');
			Right = Application::InputManager.IsPressed('D');
			Left  = Application::InputManager.IsPressed('A');
			Up	  = Application::InputManager.IsPressed('E');
			Down  = Application::InputManager.IsPressed('Q');
		}

		if (Application::InputManager.RawInputEnabled)
		{
			float z = CameraComponent.MovementSpeed * ((Fwd * DeltaTime) + (Bwd * -DeltaTime));
			float x = CameraComponent.StrafeSpeed * ((Right * DeltaTime) + (Left * -DeltaTime));
			float y = CameraComponent.StrafeSpeed * ((Up * DeltaTime) + (Down * -DeltaTime));

			if (CameraComponent.Momentum)
			{
				ApplyMomentum(LastForward, z, DeltaTime);
				ApplyMomentum(LastStrafe, x, DeltaTime);
				ApplyMomentum(LastAscent, y, DeltaTime);
			}

			CameraComponent.Translate(x, y, z);
		}

		CameraComponent.Update();
	}

	// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/CameraController.cpp
	void ApplyMomentum(float& OldValue, float& NewValue, float DeltaTime)
	{
		float BlendedValue;
		if (abs(NewValue) > abs(OldValue))
		{
			BlendedValue = lerp(NewValue, OldValue, pow(0.6f, DeltaTime * 60.0f));
		}
		else
		{
			BlendedValue = lerp(NewValue, OldValue, pow(0.8f, DeltaTime * 60.0f));
		}
		OldValue = BlendedValue;
		NewValue = BlendedValue;
	}

	CameraComponent CameraComponent;
	float			LastForward = 0.0f;
	float			LastStrafe	= 0.0f;
	float			LastAscent	= 0.0f;
};

class Editor final
	: public Application
	, public IApplicationMessageHandler
{
public:
	explicit Editor()
	{
		SetMessageHandler(this);
		RegisterMessageCallback(ImguiMessageCallback, nullptr);
	}

	bool Initialize() override
	{
		// WorldArchive::Load(ExecutableDirectory / "Assets/Scenes/cornellbox.json", World);

		std::string IniFile = (Application::ExecutableDirectory / "imgui.ini").string();
		ImGui::LoadIniSettingsFromDisk(IniFile.data());

		CreateRenderPath();

		return true;
	}

	void Shutdown() override
	{
		Renderer.reset();
	}

	void Update() override
	{
		RHI::D3D12CommandContext& Context = Kaguya::Device->GetLinkedDevice()->GetCommandContext();

		Kaguya::Device->OnBeginFrame();
		Context.Open();
		{
			Stopwatch.Signal();
			DeltaTime = static_cast<float>(Stopwatch.GetDeltaTime());
			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();
			ImGui::DockSpaceOverViewport();
			ImGui::ShowDemoWindow();
			ImGuizmo::BeginFrame();
			ImGuizmo::AllowAxisFlip(false);
			if (ImGui::Begin("Render Path"))
			{
				constexpr const char* View[] = { "Deferred Renderer", "Path Integrator DXR1.0", "Path Integrator DXR1.1" };
				if (ImGui::Combo("Render Path", &RenderPath, View, static_cast<int>(std::size(View))))
				{
					CreateRenderPath();
				}
				Renderer->OnRenderOptions();
			}
			ImGui::End();

			if (ImGui::Begin("GPU Timing"))
			{
				for (const auto& iter : Kaguya::Device->GetLinkedDevice()->GetProfiler()->Data)
				{
					for (INT i = 0; i < iter.Depth; ++i)
					{
						ImGui::Text("    ");
						ImGui::SameLine();
					}
					ImGui::Text("%s: %.2fms (%.2fms max)", iter.Name.data(), iter.AverageTime, iter.MaxTime);
					ImGui::SameLine();
					ImGui::NewLine();
				}
			}
			ImGui::End();

			if (ImGui::BeginMainMenuBar())
			{
				if (ImGui::BeginMenu("File"))
				{
					FilterDesc ComDlgFS[] = { { L"Scene File", L"*.json" }, { L"All Files (*.*)", L"*.*" } };

					if (ImGui::MenuItem("Save"))
					{
						std::filesystem::path Path = FileSystem::SaveDialog(ComDlgFS);
						if (!Path.empty())
						{
							WorldArchive::Save(Path.replace_extension(".json"), World, &EditorCamera.CameraComponent, Kaguya::AssetManager);
						}
					}

					if (ImGui::MenuItem("Load"))
					{
						std::filesystem::path Path = FileSystem::OpenDialog(ComDlgFS);
						if (!Path.empty())
						{
							WorldArchive::Load(Path, World, &EditorCamera.CameraComponent, Kaguya::AssetManager);
						}
					}

					ImGui::Separator();

					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("Edit"))
				{
					if (ImGui::MenuItem("Undo", "CTRL+Z"))
					{
					}
					if (ImGui::MenuItem("Redo", "CTRL+Y", false, false))
					{
					} // Disabled item
					ImGui::Separator();
					if (ImGui::MenuItem("Cut", "CTRL+X"))
					{
					}
					if (ImGui::MenuItem("Copy", "CTRL+C"))
					{
					}
					if (ImGui::MenuItem("Paste", "CTRL+V"))
					{
					}

					ImGui::EndMenu();
				}

				ImGui::EndMainMenuBar();
			}

			if (ImGui::Begin("Editor Camera"))
			{
				bool IsEdited = false;

				DirectX::XMFLOAT4X4 World, View, Projection;

				// Dont transpose this
				XMStoreFloat4x4(&World, EditorCamera.CameraComponent.Transform.Matrix());

				float Translation[3], Rotation[3], Scale[3];
				ImGuizmo::DecomposeMatrixToComponents(reinterpret_cast<float*>(&World), Translation, Rotation, Scale);
				IsEdited |= UIWindow::RenderFloat3Control("Translation", Translation);
				IsEdited |= UIWindow::RenderFloat3Control("Rotation", Rotation);
				IsEdited |= UIWindow::RenderFloat3Control("Scale", Scale, 1.0f);
				ImGuizmo::RecomposeMatrixFromComponents(Translation, Rotation, Scale, reinterpret_cast<float*>(&World));

				XMStoreFloat4x4(&View, DirectX::XMLoadFloat4x4(&EditorCamera.CameraComponent.View));
				XMStoreFloat4x4(&Projection, DirectX::XMLoadFloat4x4(&EditorCamera.CameraComponent.Projection));

				// If we have edited the transform, update it and mark it as dirty so it will be updated on the GPU side
				IsEdited |= UIWindow::EditTransform(
					reinterpret_cast<float*>(&View),
					reinterpret_cast<float*>(&Projection),
					reinterpret_cast<float*>(&World));

				if (IsEdited)
				{
					EditorCamera.CameraComponent.Transform.SetTransform(XMLoadFloat4x4(&World));
				}

				IsEdited |= UIWindow::RenderFloatControl("Vertical FoV", &EditorCamera.CameraComponent.FoVY, CameraComponent().FoVY, 45.0f, 85.0f);
				IsEdited |= UIWindow::RenderFloatControl("Near", &EditorCamera.CameraComponent.NearZ, CameraComponent().NearZ, 0.1f, 1.0f);
				IsEdited |= UIWindow::RenderFloatControl("Far", &EditorCamera.CameraComponent.FarZ, CameraComponent().FarZ, 10.0f, 10000.0f);

				IsEdited |= UIWindow::RenderFloatControl(
					"Movement Speed",
					&EditorCamera.CameraComponent.MovementSpeed,
					CameraComponent().MovementSpeed,
					1.0f,
					1000.0f);
				IsEdited |= UIWindow::RenderFloatControl(
					"Strafe Speed",
					&EditorCamera.CameraComponent.StrafeSpeed,
					CameraComponent().StrafeSpeed,
					1.0f,
					1000.0f);
			}
			ImGui::End();

			World->Update(DeltaTime);
			EditorCamera.OnUpdate(DeltaTime);
			if (EditorCamera.CameraComponent.Dirty)
			{
				EditorCamera.CameraComponent.Dirty = false;
				World->WorldState |= EWorldState_Update;
			}

			WorldRenderView->Camera = &EditorCamera.CameraComponent;

			WorldWindow.SetContext(World);
			WorldWindow.Render();
			AssetWindow.SetContext(World);
			AssetWindow.Render();
			InspectorWindow.SetContext(World, WorldWindow.GetSelectedActor(), &EditorCamera.CameraComponent);
			InspectorWindow.Render();

			ViewportWindow.Renderer		   = Renderer.get();
			ViewportWindow.World		   = World;
			ViewportWindow.WorldRenderView = WorldRenderView;
			ViewportWindow.MainWindow	   = MainWindow;
			ViewportWindow.Context		   = &Context;
			ViewportWindow.Render();

			auto [RenderTarget, RenderTargetView] = SwapChain->GetCurrentBackBufferResource();

			Context.TransitionBarrier(RenderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET);
			{
				Context.SetViewport(SwapChain->GetViewport());
				Context.SetScissorRect(SwapChain->GetScissorRect());
				Context.SetRenderTarget({ RenderTargetView }, nullptr);
				Context.ClearRenderTarget({ RenderTargetView }, nullptr);

				// ImGui Render
				{
					D3D12ScopedEvent(Context, "ImGui Render");

					ImGui::Render();
					ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), Context.GetGraphicsCommandList());
				}
			}
			Context.TransitionBarrier(RenderTarget, D3D12_RESOURCE_STATE_PRESENT);
		}
		Context.Close();

		RendererPresent Present(Context);
		SwapChain->Present(true, Present);
		Kaguya::Device->OnEndFrame();
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
			if (MainWindow->IsUsingRawInput())
			{
				auto& CameraComponent = EditorCamera.CameraComponent;

				CameraComponent.Rotate(
					static_cast<float>(Y) * DeltaTime * CameraComponent.MouseSensitivityY,
					static_cast<float>(X) * DeltaTime * CameraComponent.MouseSensitivityX,
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

	void OnWindowResize(Window* Window, int Width, int Height) override
	{
		Window->Resize(Width, Height);
		if (Window == MainWindow)
		{
			if (SwapChain)
			{
				SwapChain->Resize(Width, Height);
			}
		}
	}

	void OnWindowMove(Window* Window, int X, int Y) override
	{
		if (Window == MainWindow)
		{
			if (SwapChain)
			{
				SwapChain->DisplayHDRSupport();
			}
		}
	}

	void CreateRenderPath()
	{
		Renderer.reset();
		if (static_cast<RENDER_PATH>(RenderPath) == RENDER_PATH::DeferredRenderer)
		{
			Renderer = std::make_unique<DeferredRenderer>(Kaguya::Device, Kaguya::Compiler);
		}
		else if (static_cast<RENDER_PATH>(RenderPath) == RENDER_PATH::PathIntegratorDXR1_0)
		{
			Renderer = std::make_unique<PathIntegratorDXR1_0>(Kaguya::Device, Kaguya::Compiler);
		}
		else if (static_cast<RENDER_PATH>(RenderPath) == RENDER_PATH::PathIntegratorDXR1_1)
		{
			Renderer = std::make_unique<PathIntegratorDXR1_1>(Kaguya::Device, Kaguya::Compiler);
		}
		// Hack: Reset raytracing info after render path is reset to ensure no BLAS are left
		Kaguya::AssetManager->GetMeshRegistry().EnumerateAsset(
			[](Asset::AssetHandle Handle, Asset::Mesh* Mesh)
			{
				Mesh->ResetRaytracingInfo();
			});
	}

	Stopwatch Stopwatch;
	Window*	  MainWindow = nullptr;

	RHI::D3D12SwapChain* SwapChain = nullptr;

	World*					  World			  = nullptr;
	WorldRenderView*		  WorldRenderView = nullptr;
	std::unique_ptr<Renderer> Renderer		  = nullptr;
	int						  RenderPath	  = 0;

	float DeltaTime;

	EditorCamera EditorCamera;

	WorldWindow		WorldWindow;
	InspectorWindow InspectorWindow;
	AssetWindow		AssetWindow;
	ViewportWindow	ViewportWindow;
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
	D3D12RHIInitializer		D3D12RHIInitializer(DeviceOptions);
	AssetManagerInitializer AssetManagerInitializer;

	RHI::D3D12SwapChain SwapChain(Kaguya::Device, MainWindow.GetWindowHandle());

	World			World(Kaguya::AssetManager);
	WorldRenderView WorldRenderView(Kaguya::Device->GetLinkedDevice());

	// MitsubaLoader::Load("Assets/Models/coffee/scene.xml", Kaguya::AssetManager, &World);
	// MitsubaLoader::Load("Assets/Models/bathroom/scene.xml", Kaguya::AssetManager, &World);
	// MitsubaLoader::Load("Assets/Models/staircase2/scene.xml", Kaguya::AssetManager, &World);

	Editor.MainWindow	   = &MainWindow;
	Editor.SwapChain	   = &SwapChain;
	Editor.World		   = &World;
	Editor.WorldRenderView = &WorldRenderView;
	Editor.RenderPath	   = static_cast<int>(RENDER_PATH::DeferredRenderer);
	// Editor.RenderPath	   = static_cast<int>(RENDER_PATH::PathIntegratorDXR1_0);
	Editor.RenderPath = static_cast<int>(RENDER_PATH::PathIntegratorDXR1_1);
	Editor.Run();
}

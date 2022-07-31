#include "System/System.h"
#include "System/Application.h"
#include "System/IApplicationMessageHandler.h"
#include "Core/World/World.h"
#include "Core/World/WorldArchive.h"
#include "DeferredRenderer.h"
#include "PathIntegratorDXR1_0.h"
#include "PathIntegratorDXR1_1.h"
#include "Globals.h"
#include "UI/WorldWindow.h"
#include "UI/InspectorWindow.h"
#include "UI/AssetWindow.h"
#include "UI/ViewportWindow.h"
#include "GUI.h"

using namespace Math;

// https://devblogs.microsoft.com/directx/gettingstarted-dx12agility/
extern "C"
{
	_declspec(dllexport) extern const UINT D3D12SDKVersion = D3D12_SDK_VERSION;
	_declspec(dllexport) extern const char* D3D12SDKPath   = ".\\D3D12\\";
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

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

	void OnRawMouseMove(float dx, float dy)
	{
		CameraComponent.Rotate(
			dy * CameraComponent.MouseSensitivityY,
			dx * CameraComponent.MouseSensitivityX,
			0.0f);
	}

	// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/CameraController.cpp
	void ApplyMomentum(float& OldValue, float& NewValue, float DeltaTime) const noexcept
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
		Callbacks += [&](HWND HWnd, UINT Message, WPARAM WParam, LPARAM LParam)
		{
			ImGui_ImplWin32_WndProcHandler(HWnd, Message, WParam, LParam);
		};
	}

	bool Initialize() override
	{
		CreateRenderPath();

		return true;
	}

	void Shutdown() override
	{
		Renderer.reset();
	}

	void Update() override
	{
		RHI::D3D12CommandContext& Context = Kaguya::Device->GetLinkedDevice()->GetGraphicsContext();

		Kaguya::Device->OnBeginFrame();
		Context.Open();
		Stopwatch.Signal();
		DeltaTime = static_cast<float>(Stopwatch.GetDeltaTime());

		Gui->Reset();

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
			if (ImGui::BeginMenu(ICON_FA_FILE " File"))
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
			if (ImGui::BeginMenu(ICON_FA_EDIT " Edit"))
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
			IsEdited |= UIWindow::RenderFloatControl("Movement Speed", &EditorCamera.CameraComponent.MovementSpeed, CameraComponent().MovementSpeed, 1.0f, 1000.0f);
			IsEdited |= UIWindow::RenderFloatControl("Strafe Speed", &EditorCamera.CameraComponent.StrafeSpeed, CameraComponent().StrafeSpeed, 1.0f, 1000.0f);
			EditorCamera.CameraComponent.Dirty = IsEdited;
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
			Gui->Render(Context);
		}
		Context.TransitionBarrier(RenderTarget, D3D12_RESOURCE_STATE_PRESENT);
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
				EditorCamera.OnRawMouseMove(static_cast<float>(X) * DeltaTime, static_cast<float>(Y) * DeltaTime);
			}
		}
	}

	void OnWindowClose(Window* Window) override
	{
		if (Window == MainWindow)
		{
			RequestExit = true;
		}
	}

	void OnWindowResize(Window* Window, int Width, int Height) override
	{
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
		Kaguya::AssetManager->GetMeshRegistry().EnumerateAsset(
			[](Asset::AssetHandle Handle, Asset::Mesh* Mesh)
			{
				Mesh->ResetRaytracingInfo();
			});
	}

	GUI* Gui = nullptr;

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
	GlobalRHIContext		  GlobalRHIContext;
	GlobalAssetManagerContext GlobalAssetManagerContext;

	Editor Editor;
	GUI	   Gui;
	Window MainWindow;
	Editor.AddWindow(nullptr, &MainWindow, WindowDesc{ .Name = L"Kaguya", .Width = 1280, .Height = 720, .InitialSize = WindowInitialSize::Maximize });
	Gui.Initialize(MainWindow.GetWindowHandle(), Kaguya::Device);

	MainWindow.Show();

	RHI::D3D12SwapChain SwapChain(Kaguya::Device, MainWindow.GetWindowHandle());

	World			World(Kaguya::AssetManager);
	WorldRenderView WorldRenderView(Kaguya::Device->GetLinkedDevice());

	Editor.Gui			   = &Gui;
	Editor.MainWindow	   = &MainWindow;
	Editor.SwapChain	   = &SwapChain;
	Editor.World		   = &World;
	Editor.WorldRenderView = &WorldRenderView;
	// Editor.RenderPath	   = static_cast<int>(RENDER_PATH::DeferredRenderer);
	// Editor.RenderPath	   = static_cast<int>(RENDER_PATH::PathIntegratorDXR1_0);
	Editor.RenderPath = static_cast<int>(RENDER_PATH::PathIntegratorDXR1_1);
	Editor.Run();
}

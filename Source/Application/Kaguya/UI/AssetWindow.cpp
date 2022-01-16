#include "AssetWindow.h"
#include <imgui_internal.h>
#include "Core/Asset/AssetManager.h"

enum AssetTextureColumnID
{
	AssetTextureColumnID_Name,
	AssetTextureColumnID_Payload,
	AssetTextureColumnID_IsCubemap,
	AssetTextureColumnCount
};

enum AssetMeshColumnID
{
	AssetMeshColumnID_Name,
	AssetMeshColumnID_Payload,
	AssetMeshColumnCount
};

void AssetWindow::OnRender()
{
	bool AddAllMeshToHierarchy = false;

	if (ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_MouseButtonRight))
	{
		{
			if (ImGui::Button("Import Mesh"))
			{
				ImGui::OpenPopup("Mesh Options");
			}

			// Always center this window when appearing
			const ImVec2 Center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
			ImGui::SetNextWindowPos(Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
			if (ImGui::BeginPopupModal("Mesh Options", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::InputFloat3("Translation", MeshOptions.Translation.data());
				ImGui::InputFloat3("Rotation", MeshOptions.Rotation.data());
				ImGui::InputFloat("Scale", &MeshOptions.UniformScale);
				float Scale[3] = { MeshOptions.UniformScale, MeshOptions.UniformScale, MeshOptions.UniformScale };
				ImGuizmo::RecomposeMatrixFromComponents(
					MeshOptions.Translation.data(),
					MeshOptions.Rotation.data(),
					Scale,
					reinterpret_cast<float*>(&MeshOptions.Matrix));

				if (ImGui::Button("Browse...", ImVec2(120, 0)))
				{
					FilterDesc ComDlgFS[] = { // { L"Mesh Files", L"*.obj;*.stl;*.ply" },
											  { L"All Files (*.*)", L"*.*" }
					};

					MeshOptions.Path = FileSystem::OpenDialog(ComDlgFS);
					if (!MeshOptions.Path.empty())
					{
						AssetManager::AsyncLoadMesh(MeshOptions);
					}

					MeshOptions = {};
					ImGui::CloseCurrentPopup();
				}
				ImGui::SetItemDefaultFocus();
				ImGui::SameLine();
				if (ImGui::Button("Cancel", ImVec2(120, 0)))
				{
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
		}
		{
			if (ImGui::Button("Import Texture"))
			{
				ImGui::OpenPopup("Texture Options");
			}

			// Always center this window when appearing
			const ImVec2 Center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
			ImGui::SetNextWindowPos(Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
			if (ImGui::BeginPopupModal("Texture Options", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::Checkbox("sRGB", &TextureOptions.sRGB);
				ImGui::Checkbox("Generate Mips", &TextureOptions.GenerateMips);

				if (ImGui::Button("Browse...", ImVec2(120, 0)))
				{
					FilterDesc ComDlgFS[] = { // { L"Image Files", L"*.dds;*.tga;*.hdr" },
											  { L"All Files (*.*)", L"*.*" }
					};

					TextureOptions.Path = FileSystem::OpenDialog(ComDlgFS);
					if (!TextureOptions.Path.empty())
					{
						AssetManager::AsyncLoadImage(TextureOptions);
					}

					TextureOptions = {};
					ImGui::CloseCurrentPopup();
				}
				ImGui::SetItemDefaultFocus();
				ImGui::SameLine();
				if (ImGui::Button("Cancel", ImVec2(120, 0)))
				{
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
		}

		if (ImGui::Button("Add all mesh to hierarchy"))
		{
			AddAllMeshToHierarchy = true;
		}

		ImGui::EndPopup();
	}

	constexpr ImGuiTableFlags TableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable |
										   ImGuiTableFlags_Hideable | ImGuiTableFlags_RowBg |
										   ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV;

	ImGui::Text("Textures");
	if (ImGui::BeginTable("TextureCache", AssetTextureColumnCount, TableFlags))
	{
		ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 0.0f, AssetTextureColumnID_Name);
		ImGui::TableSetupColumn("Payload", ImGuiTableColumnFlags_WidthStretch, 0.0f, AssetTextureColumnID_Payload);
		ImGui::TableSetupColumn("IsCubemap", ImGuiTableColumnFlags_WidthStretch, 0.0f, AssetTextureColumnID_IsCubemap);
		ImGui::TableSetupScrollFreeze(0, 1); // Make row always visible
		ImGui::TableHeadersRow();

		auto& TextureCache = AssetManager::GetTextureCache();
		ValidImageHandles.clear();
		for (auto Handle : TextureCache)
		{
			if (Handle.IsValid())
			{
				ValidImageHandles.push_back(Handle);
			}
		}

		// Demonstrate using clipper for large vertical lists
		int				 ID = 0;
		ImGuiListClipper ListClipper;
		ListClipper.Begin(static_cast<int>(ValidImageHandles.size()));
		while (ListClipper.Step())
		{
			for (int i = ListClipper.DisplayStart; i < ListClipper.DisplayEnd; ++i)
			{
				AssetHandle Handle	= ValidImageHandles[i];
				Texture*	Texture = AssetManager::GetTextureCache().GetAsset(Handle);

				ImGui::PushID(ID++);
				{
					ImGui::TableNextRow();

					ImGui::TableNextColumn();
					ImGui::TextUnformatted(Texture->Name.data());

					ImGui::TableNextColumn();
					ImGui::SmallButton("Payload");

					// Our buttons are both drag sources and drag targets here!
					if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
					{
						// Set payload to carry the index of our item (could be anything)
						ImGui::SetDragDropPayload("ASSET_IMAGE", &Handle, sizeof(AssetHandle));

						ImGui::EndDragDropSource();
					}

					ImGui::TableNextColumn();
					ImGui::Text(Texture->IsCubemap ? "true" : "false");
				}
				ImGui::PopID();
			}
		}
		ImGui::EndTable();
	}

	auto& MeshCache = AssetManager::GetMeshCache();

	if (AddAllMeshToHierarchy)
	{
		MeshCache.EnumerateAsset(
			[&](AssetHandle Handle, Mesh* Mesh)
			{
				auto  Entity	  = pWorld->CreateActor(Mesh->Name);
				auto& StaticMesh  = Entity.AddComponent<StaticMeshComponent>();
				StaticMesh.Handle = Handle;
			});
	}

	ImGui::Text("Meshes");
	if (ImGui::BeginTable("MeshCache", AssetMeshColumnCount, TableFlags))
	{
		ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 0.0f, AssetMeshColumnID_Name);
		ImGui::TableSetupColumn("Payload", ImGuiTableColumnFlags_WidthStretch, 0.0f, AssetMeshColumnID_Payload);
		ImGui::TableSetupScrollFreeze(0, 1); // Make row always visible
		ImGui::TableHeadersRow();

		ValidMeshHandles.clear();
		for (auto Handle : MeshCache)
		{
			if (Handle.IsValid())
			{
				ValidMeshHandles.push_back(Handle);
			}
		}

		// ID is needed to resolve drag and drop
		int				 ID = 0;
		ImGuiListClipper ListClipper;
		ListClipper.Begin(static_cast<int>(ValidMeshHandles.size()));
		while (ListClipper.Step())
		{
			for (int i = ListClipper.DisplayStart; i < ListClipper.DisplayEnd; i++)
			{
				AssetHandle Handle = ValidMeshHandles[i];
				Mesh*		Mesh   = AssetManager::GetMeshCache().GetAsset(Handle);

				ImGui::PushID(ID++);
				{
					ImGui::TableNextRow();

					ImGui::TableNextColumn();
					ImGui::TextUnformatted(Mesh->Name.data());

					ImGui::TableNextColumn();
					ImGui::SmallButton("Payload");

					// Our buttons are both drag sources and drag targets here!
					if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
					{
						// Set payload to carry the index of our item (could be anything)
						ImGui::SetDragDropPayload("ASSET_MESH", &Handle, sizeof(AssetHandle));

						ImGui::EndDragDropSource();
					}
				}
				ImGui::PopID();
			}
		}
		ImGui::EndTable();
	}
}

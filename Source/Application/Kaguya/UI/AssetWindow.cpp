#include "AssetWindow.h"
#include "../Globals.h"

enum AssetTextureColumnID
{
	AssetTextureColumnID_Handle,
	AssetTextureColumnID_Name,
	AssetTextureColumnID_Payload,
	AssetTextureColumnID_IsCubemap,
	AssetTextureColumnCount
};

enum AssetMeshColumnID
{
	AssetMeshColumnID_Handle,
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
				ImGui::Checkbox("Generate Meshlets", &MeshOptions.GenerateMeshlets);

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
						Kaguya::AssetManager->AsyncLoadMesh(MeshOptions);
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
			if (ImGui::Button("Import Meshes"))
			{
				ImGui::OpenPopup("Meshes Options");
			}

			// Always center this window when appearing
			const ImVec2 Center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
			ImGui::SetNextWindowPos(Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
			if (ImGui::BeginPopupModal("Meshes Options", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::Checkbox("Generate Meshlets", &MeshOptions.GenerateMeshlets);

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

					auto Paths = FileSystem::OpenDialogMultiple(ComDlgFS);
					for (const auto& Path : Paths)
					{
						MeshOptions.Path = Path;
						if (!MeshOptions.Path.empty())
						{
							Kaguya::AssetManager->AsyncLoadMesh(MeshOptions);
						}
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
						Kaguya::AssetManager->AsyncLoadImage(TextureOptions);
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
	if (ImGui::BeginTable("TextureRegistry", AssetTextureColumnCount, TableFlags))
	{
		ImGui::TableSetupColumn("Handle", ImGuiTableColumnFlags_WidthStretch, 0.0f, AssetTextureColumnID_Handle);
		ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 0.0f, AssetTextureColumnID_Name);
		ImGui::TableSetupColumn("Payload", ImGuiTableColumnFlags_WidthStretch, 0.0f, AssetTextureColumnID_Payload);
		ImGui::TableSetupColumn("IsCubemap", ImGuiTableColumnFlags_WidthStretch, 0.0f, AssetTextureColumnID_IsCubemap);
		ImGui::TableSetupScrollFreeze(0, 1); // Make row always visible
		ImGui::TableHeadersRow();

		auto& TextureRegistry = Kaguya::AssetManager->GetTextureRegistry();
		ValidImageHandles.clear();
		for (auto Handle : TextureRegistry)
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
				Asset::AssetHandle Handle  = ValidImageHandles[i];
				Asset::Texture*	   Texture = TextureRegistry.GetAsset(Handle);

				ImGui::PushID(ID++);
				{
					ImGui::TableNextRow();

					ImGui::TableNextColumn();
					ImGui::Text("Handle Id %i", Handle.Id);

					ImGui::TableNextColumn();
					ImGui::TextUnformatted(Texture->Name.data());

					ImGui::TableNextColumn();
					ImGui::SmallButton("Payload");
					//ImGui::SameLine();
					//ShowHelpMarker("Use this to drag and drop into material properties");

					// Our buttons are both drag sources and drag targets here!
					if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
					{
						// Set payload to carry the index of our item (could be anything)
						ImGui::SetDragDropPayload("ASSET_IMAGE", &Handle, sizeof(Asset::AssetHandle));

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

	auto& MeshRegistry = Kaguya::AssetManager->GetMeshRegistry();

	if (AddAllMeshToHierarchy)
	{
		MeshRegistry.EnumerateAsset(
			[&](Asset::AssetHandle Handle, Asset::Mesh* Mesh)
			{
				auto  Entity	  = pWorld->CreateActor(Mesh->Name);
				auto& StaticMesh  = Entity.AddComponent<StaticMeshComponent>();
				StaticMesh.Handle = Handle;
			});
	}

	ImGui::Text("Meshes");
	if (ImGui::BeginTable("MeshRegistry", AssetMeshColumnCount, TableFlags))
	{
		ImGui::TableSetupColumn("Handle", ImGuiTableColumnFlags_WidthStretch, 0.0f, AssetMeshColumnID_Handle);
		ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 0.0f, AssetMeshColumnID_Name);
		ImGui::TableSetupColumn("Payload", ImGuiTableColumnFlags_WidthStretch, 0.0f, AssetMeshColumnID_Payload);
		ImGui::TableSetupScrollFreeze(0, 1); // Make row always visible
		ImGui::TableHeadersRow();

		ValidMeshHandles.clear();
		for (auto Handle : MeshRegistry)
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
				Asset::AssetHandle Handle = ValidMeshHandles[i];
				Asset::Mesh*	   Mesh	  = MeshRegistry.GetAsset(Handle);

				ImGui::PushID(ID++);
				{
					ImGui::TableNextRow();

					ImGui::TableNextColumn();
					ImGui::Text("Handle Id %i", Handle.Id);

					ImGui::TableNextColumn();
					ImGui::TextUnformatted(Mesh->Name.data());

					ImGui::TableNextColumn();
					ImGui::SmallButton("Payload");
					//ImGui::SameLine();
					//ShowHelpMarker("Use this to drag and drop into mesh components");

					// Our buttons are both drag sources and drag targets here!
					if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
					{
						// Set payload to carry the index of our item (could be anything)
						ImGui::SetDragDropPayload("ASSET_MESH", &Handle, sizeof(Asset::AssetHandle));

						ImGui::EndDragDropSource();
					}
				}
				ImGui::PopID();
			}
		}
		ImGui::EndTable();
	}
}

#include "AssetWindow.h"
#include <imgui_internal.h>
#include "Core/Asset/AssetManager.h"

enum AssetColumnID
{
	AssetColumnID_Id,
	AssetColumnID_Name,
	AssetColumnID_Payload,
	AssetColumnCount
};

void AssetWindow::OnRender()
{
	bool AddAllMeshToHierarchy = false;

	if (ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_MouseButtonRight))
	{
		// if (ImGui::Button("Import new asset"))
		//{
		//	COMDLG_FILTERSPEC ComDlgFS[] = { { L"All Files (*.*)", L"*.*" } };

		//	auto Path = Application::OpenDialog(ComDlgFS);
		//	if (!Path.empty())
		//	{
		//		AssetType Type = AssetManager::GetAssetTypeFromExtension(Path);
		//		if (Type != AssetType::Unknown)
		//		{
		//			ImGui::OpenPopup("Import Options");
		//		}

		//		// Always center this window when appearing
		//		const ImVec2 Center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
		//		ImGui::SetNextWindowPos(Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		//		if (ImGui::BeginPopupModal("Import Options", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		//		{
		//			if (Type == AssetType::Mesh)
		//			{
		//				MeshImportOptions MeshOptions = {};
		//				MeshOptions.Path			  = Path;
		//				ImGui::InputFloat3("Translation", MeshOptions.Translation.data());
		//				ImGui::InputFloat3("Rotation", MeshOptions.Rotation.data());
		//				ImGui::InputFloat("Scale", &MeshOptions.UniformScale);
		//				float Scale[3] = { MeshOptions.UniformScale,
		//								   MeshOptions.UniformScale,
		//								   MeshOptions.UniformScale };
		//				ImGuizmo::RecomposeMatrixFromComponents(
		//					MeshOptions.Translation.data(),
		//					MeshOptions.Rotation.data(),
		//					Scale,
		//					reinterpret_cast<float*>(&MeshOptions.Matrix));

		//				if (ImGui::Button("Ok", ImVec2(120, 0)))
		//				{
		//					AssetManager::AsyncLoadMesh(MeshOptions);

		//					ImGui::CloseCurrentPopup();
		//				}
		//			}
		//			else if (Type == AssetType::Texture)
		//			{
		//				TextureImportOptions TextureOptions = {};
		//				TextureOptions.Path					= Path;
		//				ImGui::Checkbox("sRGB", &TextureOptions.sRGB);
		//				ImGui::Checkbox("Generate Mips", &TextureOptions.GenerateMips);
		//				if (ImGui::Button("Ok", ImVec2(120, 0)))
		//				{
		//					AssetManager::AsyncLoadImage(TextureOptions);

		//					ImGui::CloseCurrentPopup();
		//				}
		//			}

		//			ImGui::SetItemDefaultFocus();
		//			ImGui::SameLine();
		//			if (ImGui::Button("Cancel", ImVec2(120, 0)))
		//			{
		//				ImGui::CloseCurrentPopup();
		//			}
		//			ImGui::EndPopup();
		//		}
		//	}
		//}

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
				MeshImportOptions MeshOptions = {};

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
					COMDLG_FILTERSPEC ComDlgFS[] = { // { L"Mesh Files", L"*.obj;*.stl;*.ply" },
													 { L"All Files (*.*)", L"*.*" }
					};

					MeshOptions.Path = Application::OpenDialog(ComDlgFS);
					if (!MeshOptions.Path.empty())
					{
						AssetManager::AsyncLoadMesh(MeshOptions);
					}

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
				TextureImportOptions TextureOptions = {};

				ImGui::Checkbox("sRGB", &TextureOptions.sRGB);
				ImGui::Checkbox("Generate Mips", &TextureOptions.GenerateMips);

				if (ImGui::Button("Browse...", ImVec2(120, 0)))
				{
					COMDLG_FILTERSPEC ComDlgFS[] = { // { L"Image Files", L"*.dds;*.tga;*.hdr" },
													 { L"All Files (*.*)", L"*.*" }
					};

					TextureOptions.Path = Application::OpenDialog(ComDlgFS);
					if (!TextureOptions.Path.empty())
					{
						AssetManager::AsyncLoadImage(TextureOptions);
					}

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
	if (ImGui::BeginTable("TextureCache", AssetColumnCount, TableFlags))
	{
		ImGui::TableSetupColumn("Id", ImGuiTableColumnFlags_WidthFixed, 5.0f, AssetColumnID_Id);
		ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 5.0f, AssetColumnID_Name);
		ImGui::TableSetupColumn("Payload", ImGuiTableColumnFlags_WidthFixed, 5.0f, AssetColumnID_Payload);
		ImGui::TableSetupScrollFreeze(0, 1); // Make row always visible
		ImGui::TableHeadersRow();

		auto& ImageCache = AssetManager::GetTextureCache();
		ValidImageHandles.clear();
		for (auto Handle : ImageCache)
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
				ImGui::PushID(ID++);
				{
					ImGui::TableNextRow();

					AssetHandle Handle = ValidImageHandles[i];

					ImGui::TableNextColumn();
					UINT id = Handle.Id;
					ImGui::Text("%u", id);

					ImGui::TableNextColumn();
					Texture* Texture = AssetManager::GetTextureCache().GetAsset(Handle);
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
				}
				ImGui::PopID();
			}
		}
		ImGui::EndTable();
	}

	auto& MeshCache = AssetManager::GetMeshCache();

	if (AddAllMeshToHierarchy)
	{
		MeshCache.Each(
			[&](AssetHandle Handle, Mesh* Mesh)
			{
				auto Entity = pWorld->CreateEntity(Mesh->Name);
				Entity.AddComponent<MeshFilter>(Handle);
				Entity.AddComponent<MeshRenderer>();
			});
	}

	ImGui::Text("Meshes");
	if (ImGui::BeginTable("MeshCache", AssetColumnCount, TableFlags))
	{
		ImGui::TableSetupColumn("Id", ImGuiTableColumnFlags_WidthFixed, 5.0f, AssetColumnID_Id);
		ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 5.0f, AssetColumnID_Name);
		ImGui::TableSetupColumn("Payload", ImGuiTableColumnFlags_WidthFixed, 5.0f, AssetColumnID_Payload);
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
				ImGui::PushID(ID++);
				{
					ImGui::TableNextRow();

					AssetHandle Handle = ValidMeshHandles[i];

					ImGui::TableNextColumn();
					UINT id = Handle.Id;
					ImGui::Text("%u", id);

					ImGui::TableNextColumn();
					Mesh* Mesh = AssetManager::GetMeshCache().GetAsset(Handle);
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

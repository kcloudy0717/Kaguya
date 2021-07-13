#include "AssetWindow.h"

#include <Graphics/AssetManager.h>

enum AssetColumnID
{
	AssetColumnID_Key,
	AssetColumnID_Name,
	AssetColumnID_Payload,
	AssetColumnID_ReferenceCount,
};

void AssetWindow::RenderGui()
{
	ImGui::Begin("Asset");

	UIWindow::Update();

	bool AddAllMeshToHierarchy = false;

	if (ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_MouseButtonRight))
	{
		// TODO: Write a function for this
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
				static bool sRGB = true;
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
				ImGui::Checkbox("sRGB", &sRGB);
				ImGui::PopStyleVar();

				if (ImGui::Button("Browse...", ImVec2(120, 0)))
				{
					OpenDialogMultiple(
						"dds,tga,hdr",
						"",
						[&](auto Path)
						{
							AssetManager::AsyncLoadImage(Path, sRGB);
						});

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
			if (ImGui::Button("Import Mesh"))
			{
				ImGui::OpenPopup("Mesh Options");
			}

			// Always center this window when appearing
			const ImVec2 Center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
			ImGui::SetNextWindowPos(Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
			if (ImGui::BeginPopupModal("Mesh Options", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			{
				static bool KeepGeometryInRAM = true;
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
				ImGui::Checkbox("Keep Geometry In RAM", &KeepGeometryInRAM);
				ImGui::PopStyleVar();

				if (ImGui::Button("Browse...", ImVec2(120, 0)))
				{
					OpenDialogMultiple(
						"obj,stl,ply",
						"",
						[&](auto Path)
						{
							AssetManager::AsyncLoadMesh(Path, KeepGeometryInRAM);
						});

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

	ImGui::Text("Images");
	if (ImGui::BeginTable("ImageCache", 4, TableFlags))
	{
		auto& ImageCache = AssetManager::GetImageCache();

		ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, 5.0f, AssetColumnID_Key);
		ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 5.0f, AssetColumnID_Name);
		ImGui::TableSetupColumn("Payload", ImGuiTableColumnFlags_WidthFixed, 5.0f, AssetColumnID_Payload);
		ImGui::TableSetupColumn(
			"Reference Count",
			ImGuiTableColumnFlags_WidthFixed,
			5.0f,
			AssetColumnID_ReferenceCount);
		ImGui::TableSetupScrollFreeze(0, 1); // Make row always visible
		ImGui::TableHeadersRow();

		// Demonstrate using clipper for large vertical lists
		ImGuiListClipper ListClipper;
		ListClipper.Begin(ImageCache.size());

		auto iter = ImageCache.Cache.begin();
		while (ListClipper.Step())
		{
			for (int row = ListClipper.DisplayStart; row < ListClipper.DisplayEnd; row++, iter++)
			{
				ImGui::PushID(iter->first);
				{
					ImGui::TableNextRow();

					ImGui::TableNextColumn();
					ImGui::Text("%llu", iter->first);

					ImGui::TableNextColumn();
					ImGui::TextUnformatted(iter->second->Name.data());

					ImGui::TableNextColumn();
					ImGui::SmallButton("Payload");

					// Our buttons are both drag sources and drag targets here!
					if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
					{
						// Set payload to carry the index of our item (could be anything)
						ImGui::SetDragDropPayload("ASSET_IMAGE", &iter->first, sizeof(UINT64));

						ImGui::EndDragDropSource();
					}

					ImGui::TableNextColumn();
					ImGui::Text("%d", iter->second.use_count());
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
			[&](UINT64 Key, AssetHandle<Asset::Mesh> Resource)
			{
				auto  Entity			  = pWorld->CreateEntity(Resource->Name);
				auto& MeshFilterComponent = Entity.AddComponent<MeshFilter>();
				MeshFilterComponent.Key	  = Key;

				auto& MeshRendererComponent = Entity.AddComponent<MeshRenderer>();
			});
	}

	ImGui::Text("Meshes");
	if (ImGui::BeginTable("MeshCache", 4, TableFlags))
	{
		ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, 5.0f, AssetColumnID_Key);
		ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 5.0f, AssetColumnID_Name);
		ImGui::TableSetupColumn("Payload", ImGuiTableColumnFlags_WidthFixed, 5.0f, AssetColumnID_Payload);
		ImGui::TableSetupColumn(
			"Reference Count",
			ImGuiTableColumnFlags_WidthFixed,
			5.0f,
			AssetColumnID_ReferenceCount);
		ImGui::TableSetupScrollFreeze(0, 1); // Make row always visible
		ImGui::TableHeadersRow();

		// Demonstrate using clipper for large vertical lists
		ImGuiListClipper ListClipper;
		ListClipper.Begin(MeshCache.size());

		auto iter = MeshCache.Cache.begin();
		while (ListClipper.Step())
		{
			for (int row = ListClipper.DisplayStart; row < ListClipper.DisplayEnd; row++, iter++)
			{
				ImGui::PushID(iter->first);
				{
					ImGui::TableNextRow();

					ImGui::TableNextColumn();
					ImGui::Text("%llu", iter->first);

					ImGui::TableNextColumn();
					ImGui::TextUnformatted(iter->second->Name.data());

					ImGui::TableNextColumn();
					ImGui::SmallButton("Payload");

					// Our buttons are both drag sources and drag targets here!
					if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
					{
						// Set payload to carry the index of our item (could be anything)
						ImGui::SetDragDropPayload("ASSET_MESH", &iter->first, sizeof(UINT64));

						ImGui::EndDragDropSource();
					}

					ImGui::TableNextColumn();
					ImGui::Text("%d", iter->second.use_count());
				}
				ImGui::PopID();
			}
		}
		ImGui::EndTable();
	}
	ImGui::End();
}

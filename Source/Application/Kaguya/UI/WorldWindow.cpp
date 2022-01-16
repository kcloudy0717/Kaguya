#include "WorldWindow.h"

#include <Core/World/WorldArchive.h>
#include "Core/CoreDefines.h"

void WorldWindow::OnRender()
{
	for (auto [Index, Actor] : enumerate(pWorld->Actors))
	{
		auto& Name = Actor.GetComponent<CoreComponent>().Name;

		ImGuiTreeNodeFlags TreeNodeFlags = (GetSelectedActor() == Actor ? ImGuiTreeNodeFlags_Selected : 0);
		TreeNodeFlags |= ImGuiTreeNodeFlags_SpanAvailWidth;
		TreeNodeFlags |= ImGuiTreeNodeFlags_Leaf;
		bool bOpened  = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)Actor, TreeNodeFlags, Name.data());
		bool bClicked = ImGui::IsItemClicked();

		if (bClicked)
		{
			SelectedIndex = Index;
		}

		if (bOpened)
		{
			ImGui::TreePop();
		}
	}

	// Right-click on blank space
	if (ImGui::BeginPopupContextWindow(
			"Hierarchy Popup Context Window",
			ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
	{
		if (ImGui::MenuItem("Create Empty"))
		{
			Actor NewActor = pWorld->CreateActor("");
			SelectedIndex  = pWorld->Actors.size() - 1;
		}

		if (SelectedIndex)
		{
			if (ImGui::MenuItem("Clone Selected"))
			{
				pWorld->CloneActor(SelectedIndex.value());
			}

			if (ImGui::MenuItem("Delete Selected"))
			{
				pWorld->DestroyActor(SelectedIndex.value());
				SelectedIndex = {};
			}
		}

		if (ImGui::MenuItem("Clear"))
		{
			pWorld->Clear();
			SelectedIndex = std::nullopt;
		}

		FilterDesc ComDlgFS[] = { { L"Scene File", L"*.json" }, { L"All Files (*.*)", L"*.*" } };

		if (ImGui::MenuItem("Save"))
		{
			std::filesystem::path Path = FileSystem::SaveDialog(ComDlgFS);
			if (!Path.empty())
			{
				WorldArchive::Save(Path.replace_extension(".json"), pWorld);
			}
		}

		if (ImGui::MenuItem("Load"))
		{
			std::filesystem::path Path = FileSystem::OpenDialog(ComDlgFS);
			if (!Path.empty())
			{
				WorldArchive::Load(Path, pWorld);
			}
		}

		ImGui::EndPopup();
	}
}

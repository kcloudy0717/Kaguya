#include "WorldWindow.h"

#include <World/WorldArchive.h>

void WorldWindow::OnRender()
{
	ImGui::Begin("World Options");
	{
		static bool Play = false;
		if (ImGui::Checkbox("Play", &Play))
		{
			if (Play)
			{
				pWorld->SimulatePhysics = true;
				pWorld->BeginPlay();
			}
			else
			{
				pWorld->SimulatePhysics = false;
			}
		}
	}
	ImGui::End();

	for (auto [Index, Entity] : enumerate(pWorld->Entities))
	{
		auto& Name = Entity.GetComponent<CoreComponent>().Name;

		ImGuiTreeNodeFlags TreeNodeFlags = (GetSelectedEntity() == Entity ? ImGuiTreeNodeFlags_Selected : 0);
		TreeNodeFlags |= ImGuiTreeNodeFlags_SpanAvailWidth;
		TreeNodeFlags |= ImGuiTreeNodeFlags_Leaf;
		bool bOpened  = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)Entity, TreeNodeFlags, Name.data());
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
			Entity NewEntity = pWorld->CreateEntity("");
			SelectedIndex	 = pWorld->Entities.size() - 1;
		}

		if (SelectedIndex)
		{
			if (ImGui::MenuItem("Clone Selected"))
			{
				pWorld->CloneEntity(SelectedIndex.value());
			}

			if (ImGui::MenuItem("Delete Selected"))
			{
				pWorld->DestroyEntity(SelectedIndex.value());
				SelectedIndex = {};
			}
		}

		if (ImGui::MenuItem("Clear"))
		{
			pWorld->Clear();
			SelectedIndex = std::nullopt;
		}

		COMDLG_FILTERSPEC ComDlgFS[] = { { L"Scene File", L"*.json" }, { L"All Files (*.*)", L"*.*" } };

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

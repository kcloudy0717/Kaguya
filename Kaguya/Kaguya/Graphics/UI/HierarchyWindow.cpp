#include "HierarchyWindow.h"

#include <World/WorldParser.h>

void HierarchyWindow::OnRender()
{
	pWorld->Registry.each(
		[&](auto Handle)
		{
			Entity Entity(Handle, pWorld);

			auto& Name = Entity.GetComponent<Tag>().Name;

			ImGuiTreeNodeFlags TreeNodeFlags = (SelectedEntity == Entity ? ImGuiTreeNodeFlags_Selected : 0);
			TreeNodeFlags |= ImGuiTreeNodeFlags_SpanAvailWidth;
			TreeNodeFlags |= ImGuiTreeNodeFlags_Leaf;
			bool bOpened  = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)Entity, TreeNodeFlags, Name.data());
			bool bClicked = ImGui::IsItemClicked();

			if (bClicked)
			{
				SelectedEntity = Entity;
			}

			if (bOpened)
			{
				ImGui::TreePop();
			}
		});

	// Right-click on blank space
	if (ImGui::BeginPopupContextWindow(
			"Hierarchy Popup Context Window",
			ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
	{
		if (ImGui::MenuItem("Create Empty"))
		{
			Entity NewEntity = pWorld->CreateEntity("");
			SelectedEntity	 = NewEntity;
		}

		if (SelectedEntity)
		{
			if (ImGui::MenuItem("Delete Selected"))
			{
				pWorld->DestroyEntity(SelectedEntity);
			}
		}

		if (ImGui::MenuItem("Clear"))
		{
			pWorld->Clear();
			SelectedEntity = {};
		}

		COMDLG_FILTERSPEC ComDlgFS[] = { { L"Scene File", L"*.json" }, { L"All Files (*.*)", L"*.*" } };

		if (ImGui::MenuItem("Save"))
		{
			std::filesystem::path Path = Application::SaveDialog(ComDlgFS);
			if (!Path.empty())
			{
				WorldParser::Save(Path.replace_extension(".json"), pWorld);
			}
		}

		if (ImGui::MenuItem("Load"))
		{
			std::filesystem::path Path = Application::OpenDialog(ComDlgFS);
			if (!Path.empty())
			{
				WorldParser::Load(Path, pWorld);
			}
		}

		ImGui::EndPopup();
	}
}

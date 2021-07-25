#include "HierarchyWindow.h"

#include <World/SceneParser.h>
#include <World/WorldParser.h>

void HierarchyWindow::RenderGui()
{
	ImGui::Begin("Hierarchy");

	UIWindow::Update();

	pWorld->Registry.each(
		[&](auto Handle)
		{
			Entity Entity(Handle, pWorld);

			auto& Name = Entity.GetComponent<Tag>().Name;

			ImGuiTreeNodeFlags TreeNodeFlags =
				((SelectedEntity == Entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_SpanAvailWidth;
			bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)Entity, TreeNodeFlags, Name.data());
			if (ImGui::IsItemClicked())
			{
				SelectedEntity = Entity;
			}

			bool Delete = false;
			if (ImGui::BeginPopupContextItem())
			{
				if (ImGui::MenuItem("Delete"))
				{
					Delete = true;
				}

				ImGui::EndPopup();
			}

			if (opened)
			{
				ImGui::TreePop();
			}

			if (Delete)
			{
				pWorld->DestroyEntity(Entity);
				if (SelectedEntity == Entity)
				{
					SelectedEntity = {};
				}
			}
		});

	// Right-click on blank space
	if (ImGui::BeginPopupContextWindow(
			"Hierarchy Popup Context Window",
			ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
	{
		if (ImGui::MenuItem("Create Empty"))
		{
			Entity entity  = pWorld->CreateEntity("GameObject");
			SelectedEntity = entity;
		}

		if (ImGui::MenuItem("Clear"))
		{
			pWorld->Clear();
			SelectedEntity = {};
		}

		COMDLG_FILTERSPEC ComDlgFS[] = { { L"Scene File", L"*.yaml;*.json" }, { L"All Files (*.*)", L"*.*" } };

		if (ImGui::MenuItem("Save"))
		{
			std::filesystem::path Path = Application::SaveDialog(2, ComDlgFS);
			if (!Path.empty())
			{
				WorldParser::Save(Path.replace_extension(".json"), pWorld);
			}
		}

		if (ImGui::MenuItem("Load"))
		{
			std::filesystem::path Path = Application::OpenDialog(2, ComDlgFS);
			if (!Path.empty())
			{
				//SceneParser::Load(Path, pWorld);
				WorldParser::Load(Path, pWorld);
			}
		}

		ImGui::EndPopup();
	}
	ImGui::End();

	// Reset entity is nothing is clicked
	if (ImGui::IsItemClicked())
	{
		SelectedEntity = {};
	}
}

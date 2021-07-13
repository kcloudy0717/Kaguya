#include "HierarchyWindow.h"

#include <World/SceneParser.h>

void HierarchyWindow::RenderGui()
{
	ImGui::Begin("Hierarchy");

	UIWindow::Update();

	pWorld->Registry.each(
		[&](auto Handle)
		{
			Entity Entity(Handle, pWorld);

			auto& Name = Entity.GetComponent<Tag>().Name;

			ImGuiTreeNodeFlags TreeNodeFlags = ((SelectedEntity == Entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_SpanAvailWidth;
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

		if (ImGui::MenuItem("Save"))
		{
			SaveDialog(
				"yaml",
				"",
				[&](auto Path)
				{
					if (!Path.has_extension())
					{
						Path += ".yaml";
					}

					SceneParser::Save(Path, pWorld);
				});
		}

		if (ImGui::MenuItem("Load"))
		{
			OpenDialog(
				"yaml",
				"",
				[&](auto Path)
				{
					SceneParser::Load(Path, pWorld);
				});
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

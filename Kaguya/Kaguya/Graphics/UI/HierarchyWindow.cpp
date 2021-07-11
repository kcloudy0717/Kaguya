#include "HierarchyWindow.h"

#include <Graphics/Scene/SceneParser.h>

void HierarchyWindow::RenderGui()
{
	ImGui::Begin("Hierarchy");

	UIWindow::Update();

	pScene->Registry.each(
		[&](auto Handle)
		{
			Entity entity(Handle, pScene);

			auto& Name = entity.GetComponent<Tag>().Name;

			ImGuiTreeNodeFlags flags = ((SelectedEntity == entity) ? ImGuiTreeNodeFlags_Selected : 0);
			flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
			bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, Name.data());
			if (ImGui::IsItemClicked())
			{
				SelectedEntity = entity;
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
				pScene->DestroyEntity(entity);
				if (SelectedEntity == entity)
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
			Entity entity	 = pScene->CreateEntity("GameObject");
			SelectedEntity = entity;
		}

		if (ImGui::MenuItem("Clear"))
		{
			pScene->Clear();
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

					SceneParser::Save(Path, pScene);
				});
		}

		if (ImGui::MenuItem("Load"))
		{
			OpenDialog(
				"yaml",
				"",
				[&](auto Path)
				{
					SceneParser::Load(Path, pScene);
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

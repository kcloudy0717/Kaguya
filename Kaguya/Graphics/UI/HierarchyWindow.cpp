#include "pch.h"
#include "HierarchyWindow.h"

#include <Graphics/Scene/SceneParser.h>

void HierarchyWindow::RenderGui()
{
	ImGui::Begin("Hierarchy");

	UIWindow::Update();

	m_pScene->Registry.each(
		[&](auto Handle)
		{
			Entity entity(Handle, m_pScene);

			auto& Name = entity.GetComponent<Tag>().Name;

			ImGuiTreeNodeFlags flags = ((m_SelectedEntity == entity) ? ImGuiTreeNodeFlags_Selected : 0);
			flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
			bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, Name.data());
			if (ImGui::IsItemClicked())
			{
				m_SelectedEntity = entity;
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
				m_pScene->DestroyEntity(entity);
				if (m_SelectedEntity == entity)
				{
					m_SelectedEntity = {};
				}
			}
		});

	// Right-click on blank space
	if (ImGui::BeginPopupContextWindow("Hierarchy Popup Context Window", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
	{
		if (ImGui::MenuItem("Create Empty"))
		{
			Entity entity = m_pScene->CreateEntity("GameObject");
			m_SelectedEntity = entity;
		}

		if (ImGui::MenuItem("Clear"))
		{
			m_pScene->Clear();
			m_SelectedEntity = {};
		}

		if (ImGui::MenuItem("Save"))
		{
			SaveDialog("yaml", "",
				[&](auto Path)
				{
					if (!Path.has_extension())
					{
						Path += ".yaml";
					}

					SceneParser::Save(Path, m_pScene);
				});
		}

		if (ImGui::MenuItem("Load"))
		{
			OpenDialog("yaml", "",
				[&](auto Path)
				{
					SceneParser::Load(Path, m_pScene);
				});
		}

		ImGui::EndPopup();
	}
	ImGui::End();

	// Reset entity is nothing is clicked
	if (ImGui::IsItemClicked())
	{
		m_SelectedEntity = {};
	}
}

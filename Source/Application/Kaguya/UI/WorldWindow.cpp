#include "WorldWindow.h"

void WorldWindow::OnRender()
{
	for (size_t i = 0; i < pWorld->Actors.size(); ++i)
	{
		auto& Actor = pWorld->Actors[i];
		auto& Name	= Actor.GetComponent<CoreComponent>().Name;

		ImGuiTreeNodeFlags TreeNodeFlags = (GetSelectedActor() == Actor ? ImGuiTreeNodeFlags_Selected : 0);
		TreeNodeFlags |= ImGuiTreeNodeFlags_SpanAvailWidth;
		TreeNodeFlags |= ImGuiTreeNodeFlags_Leaf;
		bool bOpened  = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)Actor, TreeNodeFlags, Name.data());
		bool bClicked = ImGui::IsItemClicked();

		if (bClicked)
		{
			SelectedIndex = i;
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

		ImGui::EndPopup();
	}
}

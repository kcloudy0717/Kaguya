#include "UIWindow.h"
#include <imgui_internal.h>
#include <ImGuizmo.h>

void UIWindow::Render()
{
	if (ImGui::Begin(Name.data(), nullptr, Flags))
	{
		IsHovered = ImGui::IsWindowHovered();

		OnRender();
	}
	ImGui::End();
}

bool UIWindow::RenderButtonDragFloatControl(
	std::string_view ButtonLabel,
	std::string_view DragFloatLabel,
	float*			 Float,
	float			 ResetValue,
	float			 Min,
	float			 Max,
	ImVec4			 ButtonColor,
	ImVec4			 ButtonHoveredColor,
	ImVec4			 ButtonActiveColor)
{
	bool IsEdited = false;

	ImFont* BoldFont   = ImGui::GetIO().Fonts->Fonts[0];
	float	LineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
	ImVec2	ButtonSize = { LineHeight + 3.0f, LineHeight };

	ImGui::PushStyleColor(ImGuiCol_Button, ButtonColor);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ButtonHoveredColor);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ButtonActiveColor);
	ImGui::PushFont(BoldFont);
	if (ImGui::Button(ButtonLabel.data(), ButtonSize))
	{
		*Float = ResetValue;
		IsEdited |= true;
	}
	ImGui::PopFont();
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	IsEdited |= ImGui::DragFloat(DragFloatLabel.data(), Float, 0.1f, Min, Max);
	ImGui::PopItemWidth();

	return IsEdited;
}

bool UIWindow::RenderFloatControl(
	std::string_view Label,
	float*			 Float,
	float			 ResetValue /*= 0.0f*/,
	float			 Min /*= 0.0f*/,
	float			 Max /*= 0.0f*/,
	float			 Width /*= ImGui::CalcItemWidth()*/)
{
	bool IsEdited = false;
	if (ImGui::BeginTable(Label.data(), 2, ImGuiTableFlags_BordersInnerV))
	{
		ImGui::TableNextRow();

		//==============================
		ImGui::TableSetColumnIndex(0);
		ImGui::Text(Label.data());

		ImGui::PushMultiItemsWidths(1, Width);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		//==============================
		ImGui::TableSetColumnIndex(1);
		IsEdited |= RenderButtonDragFloatControl(
			"X",
			"##X",
			&Float[0],
			ResetValue,
			Min,
			Max,
			ImVec4{ 0.9f, 0.1f, 0.1f, 1.0f },
			ImVec4{ 1.0f, 0.2f, 0.2f, 1.0f },
			ImVec4{ 1.0f, 0.2f, 0.2f, 1.0f });

		ImGui::PopStyleVar();

		ImGui::EndTable();
	}
	return IsEdited;
}

bool UIWindow::RenderFloat2Control(
	std::string_view Label,
	float*			 Float2,
	float			 ResetValue /*= 0.0f*/,
	float			 Min /*= 0.0f*/,
	float			 Max /*= 0.0f*/,
	float			 Width /*= ImGui::CalcItemWidth()*/)
{
	bool IsEdited = false;
	if (ImGui::BeginTable(Label.data(), 3, ImGuiTableFlags_BordersInnerV))
	{
		ImGui::TableNextRow();

		//==============================
		ImGui::TableSetColumnIndex(0);
		ImGui::Text(Label.data());

		ImGui::PushMultiItemsWidths(2, Width);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		//==============================
		ImGui::TableSetColumnIndex(1);
		IsEdited |= RenderButtonDragFloatControl(
			"X",
			"##X",
			&Float2[0],
			ResetValue,
			Min,
			Max,
			ImVec4{ 0.9f, 0.1f, 0.1f, 1.0f },
			ImVec4{ 1.0f, 0.2f, 0.2f, 1.0f },
			ImVec4{ 1.0f, 0.2f, 0.2f, 1.0f });

		//==============================
		ImGui::TableSetColumnIndex(2);
		IsEdited |= RenderButtonDragFloatControl(
			"Y",
			"##Y",
			&Float2[1],
			ResetValue,
			Min,
			Max,
			ImVec4{ 0.1f, 0.9f, 0.1f, 1.0f },
			ImVec4{ 0.2f, 1.0f, 0.2f, 1.0f },
			ImVec4{ 0.2f, 1.0f, 0.2f, 1.0f });

		ImGui::PopStyleVar();

		ImGui::EndTable();
	}
	return IsEdited;
}

bool UIWindow::RenderFloat3Control(
	std::string_view Label,
	float*			 Float3,
	float			 ResetValue /*= 0.0f*/,
	float			 Min /*= 0.0f*/,
	float			 Max /*= 0.0f*/,
	float			 Width /*= ImGui::CalcItemWidth()*/)
{
	bool IsEdited = false;
	if (ImGui::BeginTable(Label.data(), 4, ImGuiTableFlags_BordersInnerV))
	{
		ImGui::TableNextRow();

		//==============================
		ImGui::TableSetColumnIndex(0);
		ImGui::Text(Label.data());

		ImGui::PushMultiItemsWidths(3, Width);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		//==============================
		ImGui::TableSetColumnIndex(1);
		IsEdited |= RenderButtonDragFloatControl(
			"X",
			"##X",
			&Float3[0],
			ResetValue,
			Min,
			Max,
			ImVec4{ 0.9f, 0.1f, 0.1f, 1.0f },
			ImVec4{ 1.0f, 0.2f, 0.2f, 1.0f },
			ImVec4{ 1.0f, 0.2f, 0.2f, 1.0f });

		//==============================
		ImGui::TableSetColumnIndex(2);
		IsEdited |= RenderButtonDragFloatControl(
			"Y",
			"##Y",
			&Float3[1],
			ResetValue,
			Min,
			Max,
			ImVec4{ 0.1f, 0.9f, 0.1f, 1.0f },
			ImVec4{ 0.2f, 1.0f, 0.2f, 1.0f },
			ImVec4{ 0.2f, 1.0f, 0.2f, 1.0f });

		//==============================
		ImGui::TableSetColumnIndex(3);
		IsEdited |= RenderButtonDragFloatControl(
			"Z",
			"##Z",
			&Float3[2],
			ResetValue,
			Min,
			Max,
			ImVec4{ 0.1f, 0.1f, 0.9f, 1.0f },
			ImVec4{ 0.2f, 0.2f, 1.0f, 1.0f },
			ImVec4{ 0.2f, 0.2f, 1.0f, 1.0f });

		ImGui::PopStyleVar();

		ImGui::EndTable();
	}
	return IsEdited;
}

bool UIWindow::EditTransform(const float* ViewMatrix, float* ProjectMatrix, float* TransformMatrix)
{
	static bool				   UseSnap				 = false;
	static ImGuizmo::OPERATION CurrentGizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
	static float			   Snap[3]				 = { 1, 1, 1 };
	static ImGuizmo::MODE	   CurrentGizmoMode		 = ImGuizmo::WORLD;

	ImGui::Text("Operation");

	if (ImGui::RadioButton("Translate", CurrentGizmoOperation == ImGuizmo::TRANSLATE))
	{
		CurrentGizmoOperation = ImGuizmo::TRANSLATE;
	}
	ImGui::SameLine();

	if (ImGui::RadioButton("Rotate", CurrentGizmoOperation == ImGuizmo::ROTATE))
	{
		CurrentGizmoOperation = ImGuizmo::ROTATE;
	}
	ImGui::SameLine();

	if (ImGui::RadioButton("Scale", CurrentGizmoOperation == ImGuizmo::SCALE))
	{
		CurrentGizmoOperation = ImGuizmo::SCALE;
	}

	ImGui::Checkbox("Snap", &UseSnap);
	ImGui::SameLine();

	switch (CurrentGizmoOperation)
	{
	case ImGuizmo::TRANSLATE:
		ImGui::InputFloat3("Snap", &Snap[0]);
		break;
	case ImGuizmo::ROTATE:
		ImGui::InputFloat("Angle Snap", &Snap[0]);
		break;
	case ImGuizmo::SCALE:
		ImGui::InputFloat("Scale Snap", &Snap[0]);
		break;
	default:
		break;
	}

	return ImGuizmo::Manipulate(
		ViewMatrix,
		ProjectMatrix,
		CurrentGizmoOperation,
		CurrentGizmoMode,
		TransformMatrix,
		nullptr,
		UseSnap ? Snap : nullptr);
}

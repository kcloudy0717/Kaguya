#pragma once
#include <string_view>
#include <imgui.h>

class UIWindow
{
public:
	explicit UIWindow(std::string_view Name, ImGuiWindowFlags Flags)
		: Name(Name)
		, Flags(Flags)
	{
	}

	virtual ~UIWindow() = default;

	void Render();

	// Helper to display a (?) mark that shows a tooltip when hovered
	static void ShowHelpMarker(const char* Desc)
	{
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.f);
			ImGui::TextUnformatted(Desc);
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}
	}

	static bool RenderButtonDragFloatControl(
		std::string_view ButtonLabel,
		std::string_view DragFloatLabel,
		float*			 Float,
		float			 ResetValue,
		float			 Min,
		float			 Max,
		ImVec4			 ButtonColor,
		ImVec4			 ButtonHoveredColor,
		ImVec4			 ButtonActiveColor);

	static bool RenderFloatControl(
		std::string_view Label,
		float*			 Float,
		float			 ResetValue = 0.0f,
		float			 Min		= 0.0f,
		float			 Max		= 0.0f,
		float			 Width		= ImGui::CalcItemWidth());

	static bool RenderFloat2Control(
		std::string_view Label,
		float*			 Float2,
		float			 ResetValue = 0.0f,
		float			 Min		= 0.0f,
		float			 Max		= 0.0f,
		float			 Width		= ImGui::CalcItemWidth());

	static bool RenderFloat3Control(
		std::string_view Label,
		float*			 Float3,
		float			 ResetValue = 0.0f,
		float			 Min		= 0.0f,
		float			 Max		= 0.0f,
		float			 Width		= ImGui::CalcItemWidth());

	static bool EditTransform(const float* ViewMatrix, float* ProjectMatrix, float* TransformMatrix);

protected:
	virtual void OnRender() = 0;

protected:
	bool IsHovered = false;

private:
	std::string_view Name;
	ImGuiWindowFlags Flags = 0;
};

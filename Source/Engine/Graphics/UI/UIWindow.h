#pragma once

class UIWindow
{
public:
	UIWindow(const std::string& Name, ImGuiWindowFlags Flags)
		: Name(Name)
		, Flags(Flags)
	{
	}

	virtual ~UIWindow() = default;

	void Render();

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

protected:
	virtual void OnRender() = 0;

protected:
	bool IsHovered;

private:
	std::string		 Name  = "<unknown>";
	ImGuiWindowFlags Flags = 0;
};

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

protected:
	virtual void OnRender() = 0;

protected:
	bool IsHovered;

private:
	std::string		 Name  = "<Unknown>";
	ImGuiWindowFlags Flags = 0;
};

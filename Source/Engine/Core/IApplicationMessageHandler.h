#pragma once

class Window;

enum class EMouseButton
{
	Unknown,
	Left,
	Middle,
	Right
};

constexpr const char* EMouseButtonToString(EMouseButton Button)
{
	switch (Button)
	{
	case EMouseButton::Unknown:
		return "<unknown>";
	case EMouseButton::Left:
		return "Left";
	case EMouseButton::Middle:
		return "Middle";
	case EMouseButton::Right:
		return "Right";
	default:
		return "<unknown>";
	}
}

class IApplicationMessageHandler
{
public:
	virtual ~IApplicationMessageHandler() = default;

	virtual void OnKeyDown(unsigned char KeyCode, bool IsRepeat);
	virtual void OnKeyUp(unsigned char KeyCode);
	virtual void OnKeyChar(unsigned char Character, bool IsRepeat);

	virtual void OnMouseMove(Vector2i Position);
	virtual void OnMouseDown(const Window* Window, EMouseButton Button, Vector2i Position);
	virtual void OnMouseUp(EMouseButton Button, Vector2i Position);
	virtual void OnMouseDoubleClick(const Window* Window, EMouseButton Button, Vector2i Position);
	virtual void OnMouseWheel(float Delta, Vector2i Position);

	virtual void OnRawMouseMove(Vector2i Position);

	virtual void OnWindowClose(Window* Window);

	virtual void OnWindowResize(Window* Window, std::int32_t Width, std::int32_t Height);
};

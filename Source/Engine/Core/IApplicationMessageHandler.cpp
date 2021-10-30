#include "IApplicationMessageHandler.h"

void IApplicationMessageHandler::OnKeyDown(unsigned char KeyCode, bool IsRepeat)
{
#if 0
	LOG_INFO("{} ----- KeyCode: {}, IsRepeat: {}", __FUNCTION__, KeyCode, IsRepeat);
#endif
}

void IApplicationMessageHandler::OnKeyUp(unsigned char KeyCode)
{
#if 0
	LOG_INFO("{} ----- KeyCode: {}", __FUNCTION__, KeyCode);
#endif
}

void IApplicationMessageHandler::OnKeyChar(unsigned char Character, bool IsRepeat)
{
#if 0
	LOG_INFO("{} ----- Character: {}, IsRepeat: {}", __FUNCTION__, Character, IsRepeat);
#endif
}

void IApplicationMessageHandler::OnMouseMove(Vector2i Position)
{
#if 0
	LOG_INFO("{} ----- Position: {},{}", __FUNCTION__, Position.x, Position.y);
#endif
}

void IApplicationMessageHandler::OnMouseDown(const Window* Window, EMouseButton Button, Vector2i Position)
{
#if 0
	LOG_INFO(
		"{} ----- Button: {}, Position: {},{}",
		__FUNCTION__,
		EMouseButtonToString(Button),
		Position.x,
		Position.y);
#endif
}

void IApplicationMessageHandler::OnMouseUp(EMouseButton Button, Vector2i Position)
{
#if 0
	LOG_INFO(
		"{} ----- Button: {}, Position: {},{}",
		__FUNCTION__,
		EMouseButtonToString(Button),
		Position.x,
		Position.y);
#endif
}

void IApplicationMessageHandler::OnMouseDoubleClick(const Window* Window, EMouseButton Button, Vector2i Position)
{
#if 0
	LOG_INFO(
		"{} ----- Button: {}, Position: {},{}",
		__FUNCTION__,
		EMouseButtonToString(Button),
		Position.x,
		Position.y);
#endif
}

void IApplicationMessageHandler::OnMouseWheel(float Delta, Vector2i Position)
{
#if 0
	LOG_INFO("{} ----- Delta: {}, Position: {},{}", __FUNCTION__, Delta, Position.x, Position.y);
#endif
}

void IApplicationMessageHandler::OnRawMouseMove(Vector2i Position)
{
#if 0
	LOG_INFO("{} ----- Position: {},{}", __FUNCTION__, Position.x, Position.y);
#endif
}

void IApplicationMessageHandler::OnWindowClose(Window* Window)
{
#if 0
	LOG_INFO(L"{} ----- Window: {}, Name: {}", __FUNCTIONW__, fmt::ptr(Window), Window->GetDesc().Name);
#endif
}

void IApplicationMessageHandler::OnWindowResize(Window* Window, std::int32_t Width, std::int32_t Height)
{
#if 1
	LOG_INFO("{} ----- Window: {}, Resolution: {}x{}", __FUNCTION__, fmt::ptr(Window), Width, Height);
#endif
}

#pragma once
#include <bitset>
#include <optional>
#include "ThreadSafeQueue.h"

class Mouse
{
public:
	friend class InputHandler;

	enum
	{
		BufferSize = 16
	};

	enum Button
	{
		Left,
		Middle,
		Right,
		NumButtons
	};

	struct Event
	{
		enum class EType
		{
			Invalid,
			Move,
			Enter,
			Leave,
			LMBPress,
			LMBRelease,
			MMBPress,
			MMBRelease,
			RMBPress,
			RMBRelease,
			WheelUp,
			WheelDown
		};

		struct SData
		{
			int X; int Y;
			bool LeftIsPressed;
			bool MiddleIsPressed;
			bool RightIsPressed;
		};

		Event() = default;
		Event(EType Type, const Mouse& Mouse)
			: Type(Type)
		{
			Data.X = Mouse.x;
			Data.Y = Mouse.y;
			Data.LeftIsPressed = Mouse.IsLeftPressed();
			Data.MiddleIsPressed = Mouse.IsMiddlePressed();
			Data.RightIsPressed = Mouse.IsRightPressed();
		}

		EType Type = EType::Invalid;
		SData Data = {};
	};

	struct RawInput
	{
		int X, Y;
	};

	bool IsPressed(Button Button) const;
	bool IsLeftPressed() const;
	bool IsMiddlePressed() const;
	bool IsRightPressed() const;
	bool IsInWindow() const;

	std::optional<Mouse::Event> Read();

	std::optional<Mouse::RawInput> ReadRawInput();

private:
	void OnMove(int x, int y);
	void OnEnter();
	void OnLeave();

	void OnButtonDown(Button Button, int x, int y);

	void OnButtonUp(Button Button, int x, int y);

	void OnWheelDown(int x, int y);
	void OnWheelUp(int x, int y);
	void OnWheelDelta(int WheelDelta, int x, int y);

	void OnRawInput(int x, int y);

	template<class T>
	void TrimBuffer(ThreadSafeQueue<T>& QueueBuffer)
	{
		while (QueueBuffer.size() > BufferSize)
		{
			T item;
			QueueBuffer.pop(item, 0);
		}
	}

public:
	int	x = 0;
	int	y = 0;
	int	xRaw = 0;
	int	yRaw = 0;

private:
	std::bitset<NumButtons> m_ButtonStates;
	bool m_IsInWindow = false;
	int m_WheelDeltaCarry = 0;
	ThreadSafeQueue<Mouse::Event> m_MouseBuffer;
	ThreadSafeQueue<RawInput> m_RawDeltaBuffer;
};
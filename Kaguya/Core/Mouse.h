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
			bool LeftMouseButtonIsPressed;
			bool MiddleMouseButtonIsPressed;
			bool RightMouseButtonIsPressed;
		};

		Event() = default;
		Event(EType Type, const Mouse& Mouse)
			: Type(Type)
		{
			Data.X = Mouse.X;
			Data.Y = Mouse.Y;
			Data.LeftMouseButtonIsPressed = Mouse.IsLMBPressed();
			Data.MiddleMouseButtonIsPressed = Mouse.IsMMBPressed();
			Data.RightMouseButtonIsPressed = Mouse.IsRMBPressed();
		}

		EType Type = EType::Invalid;
		SData Data = {};
	};

	struct RawInput
	{
		int X, Y;
	};

	bool IsMouseButtonPressed(Button Button) const;
	bool IsLMBPressed() const;
	bool IsMMBPressed() const;
	bool IsRMBPressed() const;
	bool IsInWindow() const;

	std::optional<Mouse::Event> Read();

	std::optional<Mouse::RawInput> ReadRawInput();
private:
	void OnMouseMove(int X, int Y);
	void OnMouseEnter();
	void OnMouseLeave();

	void OnMouseButtonDown(Button Button);

	void OnMouseButtonUp(Button Button);

	void OnWheelDown();
	void OnWheelUp();
	void OnWheelDelta(int WheelDelta);

	void OnRawInput(int X, int Y);

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
	bool UseRawInput = false;
	int	X = 0;
	int	Y = 0;
	int	RawX = 0;
	int	RawY = 0;
private:
	std::bitset<NumButtons> m_ButtonStates;
	bool m_IsInWindow = false;
	int m_WheelDeltaCarry = 0;
	ThreadSafeQueue<Mouse::Event> m_MouseBuffer;
	ThreadSafeQueue<RawInput> m_RawDeltaBuffer;
};
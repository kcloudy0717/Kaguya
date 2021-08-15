#pragma once
#include <bitset>
#include <optional>
#include <queue>

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
			int	 X;
			int	 Y;
			bool LeftIsPressed;
			bool MiddleIsPressed;
			bool RightIsPressed;
		};

		Event() = default;
		Event(EType Type, const Mouse& Mouse)
			: Type(Type)
		{
			Data.X				 = Mouse.x;
			Data.Y				 = Mouse.y;
			Data.LeftIsPressed	 = Mouse.IsLeftPressed();
			Data.MiddleIsPressed = Mouse.IsMiddlePressed();
			Data.RightIsPressed	 = Mouse.IsRightPressed();
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

	std::optional<Mouse::Event> Read();

	std::optional<Mouse::RawInput> ReadRawInput();

private:
	void OnMove(int x, int y);

	void OnButtonDown(Button Button, int x, int y);

	void OnButtonUp(Button Button, int x, int y);

	void OnWheelDown(int x, int y);
	void OnWheelUp(int x, int y);
	void OnWheelDelta(int WheelDelta, int x, int y);

	void OnRawInput(int x, int y);

	template<class T>
	void TrimBuffer(std::queue<T>& QueueBuffer)
	{
		while (QueueBuffer.size() > BufferSize)
		{
			QueueBuffer.pop();
		}
	}

public:
	int x	 = 0;
	int y	 = 0;
	int xRaw = 0;
	int yRaw = 0;

private:
	std::bitset<NumButtons>	 ButtonStates;
	int						 WheelDeltaCarry = 0;
	std::queue<Mouse::Event> MouseBuffer;
	std::queue<RawInput>	 RawDeltaBuffer;
};

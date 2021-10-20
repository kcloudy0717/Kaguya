#pragma once
#include <bitset>
#include <optional>
#include <queue>

class Mouse
{
public:
	friend class InputHandler;

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

		Event() noexcept = default;
		Event(EType Type, const Mouse& Mouse) noexcept
			: Type(Type)
		{
			x				= Mouse.x;
			y				= Mouse.y;
			LeftIsPressed	= Mouse.IsLeftPressed();
			MiddleIsPressed = Mouse.IsMiddlePressed();
			RightIsPressed	= Mouse.IsRightPressed();
		}

		EType Type = EType::Invalid;
		int	  x, y;
		bool  LeftIsPressed;
		bool  MiddleIsPressed;
		bool  RightIsPressed;
	};

	struct RawInput
	{
		int x, y;
	};

	[[nodiscard]] bool IsPressed(Button Button) const;
	[[nodiscard]] bool IsLeftPressed() const;
	[[nodiscard]] bool IsMiddlePressed() const;
	[[nodiscard]] bool IsRightPressed() const;

	std::optional<Event>	Read();
	std::optional<RawInput> ReadRawInput();

private:
	void OnMove(int x, int y);

	void OnButtonDown(Button Button, int x, int y);

	void OnButtonUp(Button Button, int x, int y);

	void OnWheelDown(int x, int y);
	void OnWheelUp(int x, int y);
	void OnWheelDelta(int WheelDelta, int x, int y);

	void OnRawInput(int x, int y);

	template<class T>
	static void Trim(std::queue<T>& Queue)
	{
		static constexpr size_t BufferSize = 16;
		while (Queue.size() > BufferSize)
		{
			Queue.pop();
		}
	}

public:
	int x	 = 0;
	int y	 = 0;
	int xRaw = 0;
	int yRaw = 0;

private:
	std::bitset<NumButtons> ButtonStates;
	int						WheelDeltaCarry = 0;
	std::queue<Event>		EventQueue;
	std::queue<RawInput>	RawInputQueue;
};

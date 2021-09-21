#pragma once
#include <bitset>
#include <optional>
#include <queue>

class Keyboard
{
public:
	friend class InputHandler;

	struct Event
	{
		enum class EType
		{
			Invalid,
			Press,
			Release
		};

		Event() noexcept = default;
		Event(EType Type, unsigned char Code) noexcept
			: Type(Type)
			, Code(Code)
		{
		}

		[[nodiscard]] bool IsPressed() const noexcept { return Type == EType::Press; }

		[[nodiscard]] bool IsReleased() const noexcept { return Type == EType::Release; }

		EType		  Type = EType::Invalid;
		unsigned char Code = {};
	};

	[[nodiscard]] bool IsPressed(unsigned char KeyCode) const;

	std::optional<Event>		 Read();
	std::optional<unsigned char> ReadChar();

private:
	void ResetKeyState();
	void OnKeyDown(unsigned char KeyCode);
	void OnKeyUp(unsigned char KeyCode);
	void OnChar(unsigned char Char);

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
	bool AutoRepeat = false;

private:
	static constexpr size_t NumKeyStates = 256;

	std::bitset<NumKeyStates> KeyStates;
	std::queue<Event>		  EventQueue;
	std::queue<unsigned char> CharQueue;
};

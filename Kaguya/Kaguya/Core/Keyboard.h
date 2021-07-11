#pragma once
#include <bitset>
#include <optional>
#include <queue>

class Keyboard
{
public:
	friend class InputHandler;

	enum
	{
		BufferSize	 = 16,
		NumKeyStates = 256
	};

	struct Event
	{
		enum class EType
		{
			Invalid,
			Press,
			Release
		};

		Event() = default;
		Event(EType Type, unsigned char Code)
			: Type(Type)
			, Code(Code)
		{
		}

		bool IsPressed() const { return Type == EType::Press; }

		bool IsReleased() { return Type == EType::Release; }

		EType		  Type = EType::Invalid;
		unsigned char Code = {};
	};

	bool IsPressed(unsigned char KeyCode) const;

	std::optional<Keyboard::Event> ReadKey();
	bool						   KeyBufferIsEmpty() const;

	unsigned char ReadChar();
	bool		  CharBufferIsEmpty() const;

private:
	void ResetKeyState();
	void OnKeyDown(unsigned char KeyCode);
	void OnKeyUp(unsigned char KeyCode);
	void OnChar(unsigned char Char);

	template<class T>
	void TrimBuffer(std::queue<T>& QueueBuffer)
	{
		while (QueueBuffer.size() > BufferSize)
		{
			QueueBuffer.pop();
		}
	}

public:
	bool AutoRepeat = false;

private:
	std::bitset<NumKeyStates>	KeyStates;
	std::queue<Keyboard::Event> KeyBuffer;
	std::queue<unsigned char>	CharBuffer;
};

#include "pch.h"
#include "Keyboard.h"

bool Keyboard::IsPressed(unsigned char KeyCode) const
{
	return KeyStates[KeyCode];
}

std::optional<Keyboard::Event> Keyboard::ReadKey()
{
	if (KeyBuffer.empty())
	{
		return {};
	}

	Keyboard::Event e = KeyBuffer.front();
	KeyBuffer.pop();

	return e;
}

bool Keyboard::KeyBufferIsEmpty() const
{
	return KeyBuffer.empty();
}

unsigned char Keyboard::ReadChar()
{
	if (CharBuffer.empty())
	{
		return {};
	}

	unsigned char e = CharBuffer.front();
	CharBuffer.pop();

	return e;
}

bool Keyboard::CharBufferIsEmpty() const
{
	return CharBuffer.empty();
}

void Keyboard::ResetKeyState()
{
	KeyStates.reset();
}

void Keyboard::OnKeyDown(unsigned char KeyCode)
{
	KeyStates[KeyCode] = true;
	KeyBuffer.push(Keyboard::Event(Keyboard::Event::EType::Press, KeyCode));
	TrimBuffer(KeyBuffer);
}

void Keyboard::OnKeyUp(unsigned char KeyCode)
{
	KeyStates[KeyCode] = false;
	KeyBuffer.push(Keyboard::Event(Keyboard::Event::EType::Release, KeyCode));
	TrimBuffer(KeyBuffer);
}

void Keyboard::OnChar(unsigned char Char)
{
	CharBuffer.push(Char);
	TrimBuffer(CharBuffer);
}

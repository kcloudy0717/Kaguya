#include "Keyboard.h"

bool Keyboard::IsPressed(unsigned char KeyCode) const
{
	return KeyStates[KeyCode];
}

std::optional<Keyboard::Event> Keyboard::Read()
{
	if (EventQueue.empty())
	{
		return {};
	}

	Event Event = EventQueue.front();
	EventQueue.pop();
	return Event;
}

std::optional<unsigned char> Keyboard::ReadChar()
{
	if (CharQueue.empty())
	{
		return {};
	}

	unsigned char Char = CharQueue.front();
	CharQueue.pop();
	return Char;
}

void Keyboard::ResetKeyState()
{
	KeyStates.reset();
}

void Keyboard::OnKeyDown(unsigned char KeyCode)
{
	KeyStates[KeyCode] = true;
	EventQueue.push(Event(Event::EType::Press, KeyCode));
	Trim(EventQueue);
}

void Keyboard::OnKeyUp(unsigned char KeyCode)
{
	KeyStates[KeyCode] = false;
	EventQueue.push(Event(Event::EType::Release, KeyCode));
	Trim(EventQueue);
}

void Keyboard::OnChar(unsigned char Char)
{
	CharQueue.push(Char);
	Trim(CharQueue);
}

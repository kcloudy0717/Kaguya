#include "pch.h"
#include "Keyboard.h"

bool Keyboard::IsPressed(unsigned char KeyCode) const
{
	return m_KeyStates[KeyCode];
}

std::optional<Keyboard::Event> Keyboard::ReadKey()
{
	if (Keyboard::Event e;
		m_KeyBuffer.pop(e, 0))
	{
		return e;
	}

	return {};
}

bool Keyboard::KeyBufferIsEmpty() const
{
	return m_KeyBuffer.empty();
}

unsigned char Keyboard::ReadChar()
{
	if (unsigned char e;
		m_CharBuffer.pop(e, 0))
	{
		return e;
	}
	return NULL;
}

bool Keyboard::CharBufferIsEmpty() const
{
	return m_CharBuffer.empty();
}

void Keyboard::ResetKeyState()
{
	m_KeyStates.reset();
}

void Keyboard::OnKeyDown(unsigned char KeyCode)
{
	m_KeyStates[KeyCode] = true;
	m_KeyBuffer.push(Keyboard::Event(Keyboard::Event::EType::Press, KeyCode));
	TrimBuffer(m_KeyBuffer);
}

void Keyboard::OnKeyUp(unsigned char KeyCode)
{
	m_KeyStates[KeyCode] = false;
	m_KeyBuffer.push(Keyboard::Event(Keyboard::Event::EType::Release, KeyCode));
	TrimBuffer(m_KeyBuffer);
}

void Keyboard::OnChar(unsigned char Char)
{
	m_CharBuffer.push(Char);
	TrimBuffer(m_CharBuffer);
}

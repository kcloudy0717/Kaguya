#include "pch.h"
#include "Keyboard.h"

bool Keyboard::IsPressed(unsigned char KeyCode) const
{
	return m_KeyStates[KeyCode];
}

std::optional<Keyboard::Event> Keyboard::ReadKey()
{
	if (m_KeyBuffer.empty())
	{
		return {};
	}

	Keyboard::Event e = m_KeyBuffer.front();
	m_KeyBuffer.pop();

	return e;
}

bool Keyboard::KeyBufferIsEmpty() const
{
	return m_KeyBuffer.empty();
}

unsigned char Keyboard::ReadChar()
{
	if (m_CharBuffer.empty())
	{
		return {};
	}

	unsigned char e = m_CharBuffer.front();
	m_CharBuffer.pop();

	return e;
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

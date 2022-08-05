#include "Log.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

Log::Log(std::string_view Name)
	: StdOutputHandle(GetStdHandle(STD_OUTPUT_HANDLE))
	, Name(Name)
	, LogLevelColors{ {
		  FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,										   // Trace, White
		  FOREGROUND_GREEN | FOREGROUND_BLUE,														   // Debug, Cyan
		  FOREGROUND_GREEN,																			   // Info, Green
		  FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,									   // Warn, Intense yellow
		  FOREGROUND_RED | FOREGROUND_INTENSITY,													   // Error, Intense red
		  BACKGROUND_RED | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY, // Critical, Intense white on red background
	  } }
{
}

void Log::Write(LogLevel Level, const LogMessage& Message)
{
	if (!StdOutputHandle || StdOutputHandle == INVALID_HANDLE_VALUE)
	{
		return;
	}

	std::scoped_lock Lock{ Mutex };

	std::string	 FormattedMessage;
	const size_t MessageLength = 2 + Name.size() + 1 + Message.Payload.size() + Eol.size();
	FormattedMessage.reserve(MessageLength); // +2 for [], +1 for space
	FormattedMessage.append("[");
	FormattedMessage.append(Name);
	FormattedMessage.append("]");
	FormattedMessage.append(" ");
	FormattedMessage.append(Message.Payload);
	FormattedMessage.append(Eol);

	u16 CurrentForegroudColor = GetForegroundColor();
	SetForegroundColor(LogLevelColors[size_t(Level)]);
	std::ignore = ::WriteConsoleA(StdOutputHandle, FormattedMessage.data(), FormattedMessage.size(), nullptr, nullptr);
	SetForegroundColor(CurrentForegroudColor); // Restore foreground color
}

u16 Log::GetForegroundColor()
{
	CONSOLE_SCREEN_BUFFER_INFO Info = {};
	if (GetConsoleScreenBufferInfo(StdOutputHandle, &Info))
	{
		return Info.wAttributes & 0x0F;
	}
	return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
}

void Log::SetForegroundColor(u16 Color)
{
	CONSOLE_SCREEN_BUFFER_INFO Info = {};
	if (GetConsoleScreenBufferInfo(StdOutputHandle, &Info))
	{
		SetConsoleTextAttribute(StdOutputHandle, (Info.wAttributes & 0xF0) | Color);
	}
}

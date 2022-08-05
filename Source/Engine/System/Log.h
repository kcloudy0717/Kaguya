#pragma once
#include <array>
#include <mutex>
#include <string_view>
#include <format>
#include "Types.h"
#include "Platform.h"

enum class LogLevel
{
	Trace,
	Debug,
	Info,
	Warn,
	Error,
	Critical,
	NumLevels
};

struct LogMessage
{
	std::string_view Payload;
};

class Log
{
public:
	explicit Log(std::string_view Name);

	template<typename T, typename... TArgs>
	void Trace(const T& Message, TArgs&&... Args)
	{
		DispatchMessage(LogLevel::Trace, Message, std::forward<TArgs>(Args)...);
	}

	template<typename T, typename... TArgs>
	void Debug(const T& Message, TArgs&&... Args)
	{
		DispatchMessage(LogLevel::Debug, Message, std::forward<TArgs>(Args)...);
	}

	template<typename T, typename... TArgs>
	void Info(const T& Message, TArgs&&... Args)
	{
		DispatchMessage(LogLevel::Info, Message, std::forward<TArgs>(Args)...);
	}

	template<typename T, typename... TArgs>
	void Warn(const T& Message, TArgs&&... Args)
	{
		DispatchMessage(LogLevel::Warn, Message, std::forward<TArgs>(Args)...);
	}

	template<typename T, typename... TArgs>
	void Error(const T& Message, TArgs&&... Args)
	{
		DispatchMessage(LogLevel::Error, Message, std::forward<TArgs>(Args)...);
	}

	template<typename T, typename... TArgs>
	void Critical(const T& Message, TArgs&&... Args)
	{
		DispatchMessage(LogLevel::Critical, Message, std::forward<TArgs>(Args)...);
	}

private:
	template<typename... TArgs>
	void DispatchMessage(LogLevel Level, std::string_view Message, TArgs&&... Args)
	{
		std::string Buffer = std::vformat(Message, std::make_format_args(std::forward<TArgs>(Args)...));
		Write(Level, LogMessage{ .Payload = std::string_view(Buffer.data(), Buffer.size()) });
	}

	template<typename... TArgs>
	void DispatchMessage(LogLevel Level, std::wstring_view Message, TArgs&&... Args)
	{
		std::wstring WideBuffer = std::vformat(Message, std::make_wformat_args(std::forward<TArgs>(Args)...));
		std::string	 Buffer;
		Platform::WideToUtf8(WideBuffer, Buffer);
		Write(Level, LogMessage{ .Payload = std::string_view(Buffer.data(), Buffer.size()) });
	}

	void Write(LogLevel Level, const LogMessage& Message);

	u16	 GetForegroundColor();
	void SetForegroundColor(u16 Color);

private:
	inline static std::mutex		  Mutex;
	static constexpr std::string_view Eol = "\r\n";

	HANDLE										 StdOutputHandle = nullptr;
	std::string_view							 Name;
	std::array<u16, size_t(LogLevel::NumLevels)> LogLevelColors = {};
};

#define LOG_CONCATENATE(a, b) a##b

#define DECLARE_LOG_CATEGORY(Name)       \
	extern struct Log##Name : public Log \
	{                                    \
		Log##Name()                      \
			: Log(#Name)                 \
		{                                \
		}                                \
	} LOG_CONCATENATE(g_Log, Name)

#define DEFINE_LOG_CATEGORY(Name)	 Log##Name LOG_CONCATENATE(g_Log, Name)

#define KAGUYA_LOG(Name, Level, ...) LOG_CONCATENATE(g_Log, Name).##Level(__VA_ARGS__)

#if _DEBUG
#define KAGUYA_LOG_DEBUG(Name, Level, ...) LOG_CONCATENATE(g_Log, Name).##Level(__VA_ARGS__)
#else
#define KAGUYA_LOG_DEBUG(Name, Level, ...)
#endif

#pragma once
#define SPDLOG_WCHAR_TO_UTF8_SUPPORT
#include <spdlog/spdlog.h>

class Log
{
public:
	friend class ConsoleWindow;
	template<typename>
	friend struct imgui_sink;

	static void Initialize(const std::string& Name);

	template<typename T>
	static void Trace(const T& Msg)
	{
		s_Logger->trace(Msg);
	}

	template<typename T>
	static void Debug(const T& Msg)
	{
		s_Logger->debug(Msg);
	}

	template<typename T>
	static void Info(const T& Msg)
	{
		s_Logger->info(Msg);
	}

	template<typename T>
	static void Warn(const T& Msg)
	{
		s_Logger->warn(Msg);
	}

	template<typename T>
	static void Error(const T& Msg)
	{
		s_Logger->error(Msg);
	}

	template<typename T>
	static void Critical(const T& Msg)
	{
		s_Logger->critical(Msg);
	}

	template<typename T, typename... TArgs>
	static void Trace(const T& Msg, TArgs&&... Args)
	{
		s_Logger->trace(Msg, std::forward<TArgs>(Args)...);
	}

	template<typename T, typename... TArgs>
	static void Debug(const T& Msg, TArgs&&... Args)
	{
		s_Logger->debug(Msg, std::forward<TArgs>(Args)...);
	}

	template<typename T, typename... TArgs>
	static void Info(const T& Msg, TArgs&&... Args)
	{
		s_Logger->info(Msg, std::forward<TArgs>(Args)...);
	}

	template<typename T, typename... TArgs>
	static void Warn(const T& Msg, TArgs&&... Args)
	{
		s_Logger->warn(Msg, std::forward<TArgs>(Args)...);
	}

	template<typename T, typename... TArgs>
	static void Error(const T& Msg, TArgs&&... Args)
	{
		s_Logger->error(Msg, std::forward<TArgs>(Args)...);
	}

	template<typename T, typename... TArgs>
	static void Critical(const T& Msg, TArgs&&... Args)
	{
		s_Logger->critical(Msg, std::forward<TArgs>(Args)...);
	}

private:
	inline static std::shared_ptr<spdlog::logger> s_Logger;

	inline static ImGuiTextBuffer Buf;
	inline static ImGuiTextFilter Filter;
	inline static ImVector<int>	  LineOffsets; // Index to lines offset. We maintain this with AddLog() calls.
};

#define LOG_TRACE(...)	  Log::Trace(__VA_ARGS__)
#define LOG_INFO(...)	  Log::Info(__VA_ARGS__)
#define LOG_WARN(...)	  Log::Warn(__VA_ARGS__)
#define LOG_ERROR(...)	  Log::Error(__VA_ARGS__)
#define LOG_CRITICAL(...) Log::Critical(__VA_ARGS__)

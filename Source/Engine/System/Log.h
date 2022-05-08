#pragma once
#define SPDLOG_USE_STD_FORMAT
#define SPDLOG_WCHAR_TO_UTF8_SUPPORT
#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>

class Log
{
public:
	explicit Log(const std::string& Name);

	template<typename T>
	void Trace(const T& Msg)
	{
		Logger->trace(Msg);
	}

	template<typename T>
	void Debug(const T& Msg)
	{
		Logger->debug(Msg);
	}

	template<typename T>
	void Info(const T& Msg)
	{
		Logger->info(Msg);
	}

	template<typename T>
	void Warn(const T& Msg)
	{
		Logger->warn(Msg);
	}

	template<typename T>
	void Error(const T& Msg)
	{
		Logger->error(Msg);
	}

	template<typename T>
	void Critical(const T& Msg)
	{
		Logger->critical(Msg);
	}

	template<typename T, typename... TArgs>
	void Trace(const T& Msg, TArgs&&... Args)
	{
		Logger->trace(Msg, std::forward<TArgs>(Args)...);
	}

	template<typename T, typename... TArgs>
	void Debug(const T& Msg, TArgs&&... Args)
	{
		Logger->debug(Msg, std::forward<TArgs>(Args)...);
	}

	template<typename T, typename... TArgs>
	void Info(const T& Msg, TArgs&&... Args)
	{
		Logger->info(Msg, std::forward<TArgs>(Args)...);
	}

	template<typename T, typename... TArgs>
	void Warn(const T& Msg, TArgs&&... Args)
	{
		Logger->warn(Msg, std::forward<TArgs>(Args)...);
	}

	template<typename T, typename... TArgs>
	void Error(const T& Msg, TArgs&&... Args)
	{
		Logger->error(Msg, std::forward<TArgs>(Args)...);
	}

	template<typename T, typename... TArgs>
	void Critical(const T& Msg, TArgs&&... Args)
	{
		Logger->critical(Msg, std::forward<TArgs>(Args)...);
	}

private:
	std::shared_ptr<spdlog::logger> Logger;
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

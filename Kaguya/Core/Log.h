#pragma once
#include <spdlog/spdlog.h>

class Log
{
	friend class ConsoleWindow;
	template<typename> friend struct imgui_sink;
public:
	static void Create();

	inline static auto Instance() { return s_Logger.get(); }
private:
	inline static std::shared_ptr<spdlog::logger> s_Logger;
	
	inline static ImGuiTextBuffer Buf;
	inline static ImGuiTextFilter Filter;
	inline static ImVector<int> LineOffsets; // Index to lines offset. We maintain this with AddLog() calls.
};

#define LOG_TRACE(...) Log::Instance()->trace(__VA_ARGS__)
#define LOG_INFO(...) Log::Instance()->info(__VA_ARGS__)
#define LOG_WARN(...) Log::Instance()->warn(__VA_ARGS__)
#define LOG_ERROR(...) Log::Instance()->error(__VA_ARGS__)
#define LOG_CRITICAL(...) Log::Instance()->critical(__VA_ARGS__)
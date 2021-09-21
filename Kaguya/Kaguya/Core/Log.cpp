#include "Log.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include "spdlog/sinks/base_sink.h"

void Log::Initialize(const std::string& Name)
{
	spdlog::set_pattern("%^[%T] %n: %v%$");
	s_Logger = spdlog::stdout_color_mt(Name);
}

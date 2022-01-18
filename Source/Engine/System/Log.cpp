#include "Log.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/base_sink.h>

Log::Log(const std::string& Name)
{
	spdlog::set_pattern("%^[%T] [%n] %v%$");
	Logger = spdlog::stdout_color_mt(Name);
}

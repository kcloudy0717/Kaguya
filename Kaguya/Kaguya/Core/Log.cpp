#include "Log.h"

#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "spdlog/sinks/base_sink.h"

template<typename Mutex>
struct imgui_sink : public spdlog::sinks::base_sink<Mutex>
{
	void sink_it_(const spdlog::details::log_msg& msg) override
	{
		// log_msg is a struct containing the log entry info like level, timestamp, thread id etc.
		// msg.raw contains pre formatted log

		// If needed (very likely but not mandatory), the sink formats the message before sending it to its final
		// destination:
		spdlog::memory_buf_t formatted;
		spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
		std::string s = fmt::to_string(formatted); // formatted string

		int old_size = Log::Buf.size();
		Log::Buf.append(s.data());
		for (int new_size = Log::Buf.size(); old_size < new_size; old_size++)
			if (Log::Buf[old_size] == '\n')
				Log::LineOffsets.push_back(old_size + 1);
	}

	void flush_() override {}
};

using imgui_sink_mt = imgui_sink<std::mutex>;
using imgui_sink_st = imgui_sink<spdlog::details::null_mutex>;

void Log::Create()
{
	spdlog::set_pattern("%^[%T] %n: %v%$");
	s_Logger = spdlog::stdout_color_mt("Kaguya");
	s_Logger->sinks().push_back(std::make_shared<imgui_sink_mt>());
}

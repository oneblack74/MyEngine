#include "Core/Log.h"
#include "Core/ImGuiConsoleSink.h"

namespace Engine
{
    std::shared_ptr<spdlog::logger> Log::s_Logger;

    void Log::Init()
    {
        std::vector<spdlog::sink_ptr> sinks;

        auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        consoleSink->set_pattern("%^[%T] %n: %v%$");
        sinks.push_back(consoleSink);

        auto uiSink = std::make_shared<ImGuiConsoleSink>();
        uiSink->set_pattern("[%T] %n: %v");
        sinks.push_back(uiSink);

        s_Logger = std::make_shared<spdlog::logger>("ENGINE", sinks.begin(), sinks.end());
        s_Logger->set_level(spdlog::level::trace);
    }

    const std::deque<LogMessage> &Log::GetConsoleMessages()
    {
        return ImGuiConsoleSink::GetMessages();
    }

    uint64_t Log::GetConsoleMessageCounter()
    {
        return ImGuiConsoleSink::GetReceivedCount();
    }
}

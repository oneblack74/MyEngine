#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <deque>
#include <memory>
#include <string>

namespace Engine
{
    class Log
    {
    public:
        static void Init();
        static std::shared_ptr<spdlog::logger> &GetLogger() { return s_Logger; }

        // Derniers messages loggés, pour ConsolePanel (Editor)
        static const std::deque<std::string> &GetConsoleMessages();

    private:
        static std::shared_ptr<spdlog::logger> s_Logger;
    };
}

// Macros — non namespacées (comme d'habitude pour des macros), donc s'utilisent
// sans préfixe "Engine::" : LOG_INFO(...), pas Engine::LOG_INFO(...)
#define LOG_TRACE(...) ::Engine::Log::GetLogger()->trace(__VA_ARGS__)
#define LOG_INFO(...) ::Engine::Log::GetLogger()->info(__VA_ARGS__)
#define LOG_WARN(...) ::Engine::Log::GetLogger()->warn(__VA_ARGS__)
#define LOG_ERROR(...) ::Engine::Log::GetLogger()->error(__VA_ARGS__)
#define LOG_DEBUG(...) ::Engine::Log::GetLogger()->debug(__VA_ARGS__)
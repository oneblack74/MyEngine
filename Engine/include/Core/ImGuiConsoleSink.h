#pragma once
#include "Core/LogMessage.h"
#include <spdlog/sinks/base_sink.h>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>

namespace Engine
{
    // Sink spdlog qui garde les derniers messages formatés en mémoire, pour que
    // ConsolePanel (Editor) puisse les afficher dans l'UI en plus du terminal.
    template <typename Mutex>
    class ImGuiConsoleSinkT : public spdlog::sinks::base_sink<Mutex>
    {
    public:
        static std::deque<LogMessage> &GetMessages()
        {
            static std::deque<LogMessage> s_Messages;
            return s_Messages;
        }

        // Total de messages reçus depuis le lancement, jamais décrémenté. La taille du
        // tampon ne suffit pas à détecter un changement : une fois plein, elle reste à
        // maxMessages alors que son contenu, lui, continue de défiler.
        static uint64_t &GetReceivedCount()
        {
            static uint64_t s_ReceivedCount = 0;
            return s_ReceivedCount;
        }

    protected:
        void sink_it_(const spdlog::details::log_msg &msg) override
        {
            spdlog::memory_buf_t formatted;
            spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);

            auto &messages = GetMessages();
            // La catégorie voyage dans le nom du logger (voir Log::GetCategoryLogger).
            // Sa provenance, elle, n'est pas dupliquée par message : la console la
            // retrouve à partir du nom via Log::GetCategories().
            std::string category(msg.logger_name.data(), msg.logger_name.size());
            std::string payload(msg.payload.data(), msg.payload.size());
            messages.push_back({ToLogLevel(msg.level), std::move(category),
                                fmt::to_string(formatted), std::move(payload)});
            ++GetReceivedCount();

            constexpr size_t maxMessages = 500;
            if (messages.size() > maxMessages)
                messages.pop_front();
        }

        void flush_() override {}

    private:
        static LogLevel ToLogLevel(spdlog::level::level_enum level)
        {
            switch (level)
            {
            case spdlog::level::trace:
                return LogLevel::Trace;
            case spdlog::level::debug:
                return LogLevel::Debug;
            case spdlog::level::warn:
                return LogLevel::Warning;
            case spdlog::level::err:
                return LogLevel::Error;
            case spdlog::level::critical:
                return LogLevel::Critical;
            default:
                return LogLevel::Info;
            }
        }
    };

    using ImGuiConsoleSink = ImGuiConsoleSinkT<std::mutex>;
}

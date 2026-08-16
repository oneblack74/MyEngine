#pragma once
#include <spdlog/sinks/base_sink.h>
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
        static std::deque<std::string> &GetMessages()
        {
            static std::deque<std::string> s_Messages;
            return s_Messages;
        }

    protected:
        void sink_it_(const spdlog::details::log_msg &msg) override
        {
            spdlog::memory_buf_t formatted;
            spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);

            auto &messages = GetMessages();
            messages.emplace_back(fmt::to_string(formatted));

            constexpr size_t maxMessages = 500;
            if (messages.size() > maxMessages)
                messages.pop_front();
        }

        void flush_() override {}
    };

    using ImGuiConsoleSink = ImGuiConsoleSinkT<std::mutex>;
}

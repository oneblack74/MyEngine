#include "Core/Log.h"
#include "Core/ImGuiConsoleSink.h"
#include <mutex>
#include <unordered_map>

namespace Engine
{
    namespace
    {
        // Sinks partagés par tous les loggers : un message catégorisé doit atterrir au
        // même endroit qu'un message ordinaire, terminal et console de l'éditeur.
        std::vector<spdlog::sink_ptr> s_Sinks;

        std::mutex s_CategoryMutex;
        std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> s_CategoryLoggers;
        std::vector<LogCategoryInfo> s_Categories;

        std::shared_ptr<spdlog::logger> MakeLogger(const std::string &name)
        {
            auto logger = std::make_shared<spdlog::logger>(name, s_Sinks.begin(), s_Sinks.end());
            logger->set_level(spdlog::level::trace);
            return logger;
        }
    }

    std::shared_ptr<spdlog::logger> Log::s_Logger;

    void Log::Init()
    {
        auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        consoleSink->set_pattern("%^[%T] %n: %v%$");
        s_Sinks.push_back(consoleSink);

        auto uiSink = std::make_shared<ImGuiConsoleSink>();
        uiSink->set_pattern("[%T] %n: %v");
        s_Sinks.push_back(uiSink);

        // Pré-enregistrées pour que le menu "Moteur" de la console soit complet dès le
        // départ, au lieu de se remplir au fur et à mesure des premiers messages.
        for (const std::string *category : {&LogCategories::General, &LogCategories::Collision,
                                            &LogCategories::Input, &LogCategories::Window,
                                            &LogCategories::Renderer,
                                            &LogCategories::Physics, &LogCategories::Assets,
                                            &LogCategories::Audio})
        {
            GetCategoryLogger(LogSource::Engine, *category);
        }

        // Les LOG_* sans catégorie sont ceux de la catégorie "Général" : sans ça leurs
        // messages échapperaient aux filtres de la console, faute d'appartenir à une
        // catégorie connue.
        s_Logger = GetCategoryLogger(LogSource::Engine, LogCategories::General);
    }

    std::shared_ptr<spdlog::logger> &Log::GetCategoryLogger(LogSource source, const std::string &category)
    {
        std::lock_guard<std::mutex> lock(s_CategoryMutex);

        auto existing = s_CategoryLoggers.find(category);
        if (existing != s_CategoryLoggers.end())
            return existing->second;

        s_Categories.push_back({source, category});
        return s_CategoryLoggers.emplace(category, MakeLogger(category)).first->second;
    }

    const std::vector<LogCategoryInfo> &Log::GetCategories()
    {
        return s_Categories;
    }

    LogSource Log::GetCategorySource(const std::string &category)
    {
        std::lock_guard<std::mutex> lock(s_CategoryMutex);

        for (const LogCategoryInfo &info : s_Categories)
        {
            if (info.Name == category)
                return info.Source;
        }
        return LogSource::Engine;
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

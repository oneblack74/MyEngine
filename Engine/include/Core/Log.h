#pragma once
#include "Core/LogMessage.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <deque>
#include <memory>
#include <string>
#include <vector>

namespace Engine
{
    // Catégories du moteur. Des chaînes et non un enum : la console les traite de la
    // même façon que celles inventées par le jeu, et ajouter une catégorie ne demande
    // pas de toucher au moteur.
    namespace LogCategories
    {
        inline const std::string General = "Général";
        inline const std::string Collision = "Collision";
        inline const std::string Input = "Input";
        inline const std::string Window = "Fenêtre";
        inline const std::string Renderer = "Renderer";
        inline const std::string Physics = "Physique";
        inline const std::string Assets = "Assets";
        inline const std::string Audio = "Audio";
    }

    class Log
    {
    public:
        static void Init();
        static std::shared_ptr<spdlog::logger> &GetLogger() { return s_Logger; }

        // Un logger spdlog par catégorie, tous branchés sur les mêmes sinks. Le nom du
        // logger *est* la catégorie : elle voyage donc jusqu'au sink sans qu'il faille
        // étendre le format de message de spdlog.
        static std::shared_ptr<spdlog::logger> &GetCategoryLogger(LogSource source, const std::string &category);

        // Catégories connues, dans leur ordre d'apparition. Les catégories du moteur
        // sont enregistrées dès l'Init pour que les menus de la console ne se remplissent
        // pas au fil de l'eau ; celles du jeu apparaissent à leur première utilisation.
        static const std::vector<LogCategoryInfo> &GetCategories();

        // Provenance d'une catégorie déjà rencontrée (Engine par défaut).
        static LogSource GetCategorySource(const std::string &category);

        // Derniers messages loggés, pour ConsolePanel (Editor)
        static const std::deque<LogMessage> &GetConsoleMessages();

        // Compteur monotone de messages reçus : permet de savoir que le tampon a changé
        // même quand il est plein et que sa taille ne bouge plus.
        static uint64_t GetConsoleMessageCounter();

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

// Variantes catégorisées. La catégorie est une simple chaîne : le moteur en pré-déclare
// quelques-unes (voir Engine::LogCategories), le jeu invente les siennes librement.
//   ENGINE_LOG_INFO(Engine::LogCategories::Collision, "{0} touche {1}", a, b);
//   GAME_LOG_WARN("IA", "aucun chemin trouvé");
#define ENGINE_LOG_TRACE(category, ...) ::Engine::Log::GetCategoryLogger(::Engine::LogSource::Engine, category)->trace(__VA_ARGS__)
#define ENGINE_LOG_INFO(category, ...) ::Engine::Log::GetCategoryLogger(::Engine::LogSource::Engine, category)->info(__VA_ARGS__)
#define ENGINE_LOG_WARN(category, ...) ::Engine::Log::GetCategoryLogger(::Engine::LogSource::Engine, category)->warn(__VA_ARGS__)
#define ENGINE_LOG_ERROR(category, ...) ::Engine::Log::GetCategoryLogger(::Engine::LogSource::Engine, category)->error(__VA_ARGS__)

#define GAME_LOG_TRACE(category, ...) ::Engine::Log::GetCategoryLogger(::Engine::LogSource::Game, category)->trace(__VA_ARGS__)
#define GAME_LOG_INFO(category, ...) ::Engine::Log::GetCategoryLogger(::Engine::LogSource::Game, category)->info(__VA_ARGS__)
#define GAME_LOG_WARN(category, ...) ::Engine::Log::GetCategoryLogger(::Engine::LogSource::Game, category)->warn(__VA_ARGS__)
#define GAME_LOG_ERROR(category, ...) ::Engine::Log::GetCategoryLogger(::Engine::LogSource::Game, category)->error(__VA_ARGS__)
#pragma once
#include <string>

namespace Engine
{
    // Provenance d'un message : le moteur, ou le jeu construit par-dessus. La console
    // les filtre séparément, avec un menu déroulant pour chacune.
    enum class LogSource
    {
        Engine = 0,
        Game
    };

    // Niveau d'un message, conservé à côté du texte formaté : la console de l'éditeur
    // en a besoin pour filtrer et colorer, ce que la ligne formatée seule ne permet pas.
    enum class LogLevel
    {
        Trace = 0,
        Debug,
        Info,
        Warning,
        Error,
        Critical
    };

    struct LogMessage
    {
        LogLevel Level = LogLevel::Info;

        // Catégorie du message ("Collision", "Renderer", "Général"…). Portée par le nom
        // du logger spdlog, ce qui la fait aussi apparaître dans la ligne formatée.
        std::string Category;

        // Ligne complète, telle qu'affichée (horodatage et catégorie compris).
        std::string Text;

        // Message brut, sans horodatage : c'est lui qui sert à reconnaître deux messages
        // « identiques ». Comparer le texte formaté ne marcherait pas, deux occurrences
        // à des secondes différentes n'ayant jamais la même ligne.
        std::string Payload;
    };

    // Une catégorie connue, telle qu'affichée dans les menus de la console.
    struct LogCategoryInfo
    {
        LogSource Source = LogSource::Engine;
        std::string Name;
    };

    // Les trois catégories affichées par la console, façon Unity : tout ce qui est
    // informatif d'un côté, les avertissements, les erreurs.
    enum class LogSeverityFilter
    {
        Info = 0,
        Warning,
        Error
    };

    inline LogSeverityFilter ToSeverityFilter(LogLevel level)
    {
        switch (level)
        {
        case LogLevel::Warning:
            return LogSeverityFilter::Warning;
        case LogLevel::Error:
        case LogLevel::Critical:
            return LogSeverityFilter::Error;
        default:
            return LogSeverityFilter::Info;
        }
    }
}

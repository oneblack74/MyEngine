#pragma once
#include <string>

namespace Engine
{
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
        std::string Text;
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

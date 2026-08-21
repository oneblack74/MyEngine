#pragma once
#include <Core/LogMessage.h>
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

// Affiche les messages loggés via spdlog (Engine::Log) directement dans l'éditeur,
// avec filtrage par niveau et regroupement des messages identiques.
class ConsolePanel
{
public:
    void OnImGuiRender();

    size_t GetVisibleLineCountForTests() const { return m_VisibleLines.size(); }
    bool IsCollapsedForTests() const { return m_Collapse; }

private:
    // Une ligne telle qu'affichée : en mode groupé, elle représente toutes les
    // occurrences d'un même message et Count vaut plus que 1.
    struct DisplayLine
    {
        Engine::LogLevel Level = Engine::LogLevel::Info;
        const std::string *Text = nullptr;
        int Count = 1;
    };

    void RebuildVisibleLines();
    void RenderToolbar();
    // Un menu déroulant par provenance, avec une case par catégorie.
    void RenderCategoryMenu(const char *label, Engine::LogSource source);
    void SyncCategories();

    std::vector<DisplayLine> m_VisibleLines;

    // Regroupe les messages identiques sur une seule ligne, façon "Collapse" d'Unity.
    bool m_Collapse = false;

    // Catégories affichées, indexées par nom. Une catégorie inconnue est affichée par
    // défaut : mieux vaut voir un message inattendu que le perdre en silence.
    std::unordered_map<std::string, bool> m_CategoryEnabled;
    size_t m_KnownCategoryCount = 0;

    // Un filtre par catégorie de sévérité (Info / Warning / Error).
    bool m_ShowSeverity[3] = {true, true, true};
    int m_SeverityCounts[3] = {0, 0, 0};

    // La liste affichée n'est reconstruite que lorsqu'elle peut avoir changé, pas à
    // chaque frame : le regroupement compare des chaînes, autant ne pas le refaire
    // 60 fois par seconde pour rien.
    uint64_t m_LastMessageCounter = (uint64_t)-1;
    bool m_FiltersDirty = true;
};

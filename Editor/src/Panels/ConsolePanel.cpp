#include "Panels/ConsolePanel.h"
#include <Core/Log.h>
#include <imgui.h>
#include <unordered_map>

namespace
{
    // Couleurs de texte par niveau, façon console d'éditeur : l'informatif reste neutre,
    // l'avertissement et l'erreur se repèrent d'un coup d'œil.
    ImVec4 ColorFor(Engine::LogLevel level)
    {
        switch (Engine::ToSeverityFilter(level))
        {
        case Engine::LogSeverityFilter::Warning:
            return ImVec4(1.0f, 0.8f, 0.3f, 1.0f);
        case Engine::LogSeverityFilter::Error:
            return ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
        default:
            return ImGui::GetStyleColorVec4(ImGuiCol_Text);
        }
    }

    // Bouton à deux états : enfoncé = catégorie affichée.
    bool SeverityToggle(const char *label, bool &enabled, int count)
    {
        // "###label" fige l'ID sur le seul nom : sans ça il changerait à chaque nouveau
        // message, puisque le compteur fait partie du libellé — et ImGui perdrait l'état
        // du bouton en cours de clic.
        char text[64];
        snprintf(text, sizeof(text), "%s (%d)###%s", label, count, label);

        const ImVec4 activeColor = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
        const ImVec4 inactiveColor = ImGui::GetStyleColorVec4(ImGuiCol_FrameBg);
        ImGui::PushStyleColor(ImGuiCol_Button, enabled ? activeColor : inactiveColor);

        const bool clicked = ImGui::Button(text);
        if (clicked)
            enabled = !enabled;

        ImGui::PopStyleColor();
        return clicked;
    }
}

void ConsolePanel::OnImGuiRender()
{
    ImGui::Begin("Console");

    // Le compteur, et non la taille du tampon : celui-ci plafonne à 500 messages et
    // continue ensuite de défiler à taille constante — s'y fier laisserait m_VisibleLines
    // pointer sur des messages déjà évincés.
    SyncCategories();

    const uint64_t counter = Engine::Log::GetConsoleMessageCounter();
    if (counter != m_LastMessageCounter || m_FiltersDirty)
    {
        RebuildVisibleLines();
        m_LastMessageCounter = counter;
        m_FiltersDirty = false;
    }

    RenderToolbar();
    ImGui::Separator();

    // Zone défilante séparée : la barre d'outils doit rester visible quand la liste défile.
    if (ImGui::BeginChild("##lignes"))
    {
        for (const DisplayLine &line : m_VisibleLines)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ColorFor(line.Level));
            ImGui::TextUnformatted(line.Text->c_str());
            ImGui::PopStyleColor();

            if (line.Count > 1)
            {
                // Le compteur d'occurrences est aligné à droite, comme dans Unity.
                char badge[16];
                snprintf(badge, sizeof(badge), "%d", line.Count);
                const float badgeWidth = ImGui::CalcTextSize(badge).x + ImGui::GetStyle().FramePadding.x * 2.0f;
                ImGui::SameLine(ImGui::GetContentRegionAvail().x - badgeWidth);
                ImGui::TextDisabled("%s", badge);
            }
        }

        // Auto-scroll : ne suit le bas que si l'utilisateur n'a pas remonté manuellement
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();

    ImGui::End();
}

void ConsolePanel::RenderToolbar()
{
    if (ImGui::Checkbox("Grouper", &m_Collapse))
        m_FiltersDirty = true;
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Regroupe les messages identiques sur une seule ligne, avec leur nombre d'occurrences");

    ImGui::SameLine();
    if (SeverityToggle("Log", m_ShowSeverity[(int)Engine::LogSeverityFilter::Info],
                       m_SeverityCounts[(int)Engine::LogSeverityFilter::Info]))
        m_FiltersDirty = true;

    ImGui::SameLine();
    if (SeverityToggle("Warning", m_ShowSeverity[(int)Engine::LogSeverityFilter::Warning],
                       m_SeverityCounts[(int)Engine::LogSeverityFilter::Warning]))
        m_FiltersDirty = true;

    ImGui::SameLine();
    if (SeverityToggle("Error", m_ShowSeverity[(int)Engine::LogSeverityFilter::Error],
                       m_SeverityCounts[(int)Engine::LogSeverityFilter::Error]))
        m_FiltersDirty = true;

    // Largeur juste suffisante pour le libellé : les listes n'ont pas d'aperçu, elles
    // ne servent qu'à ouvrir leur liste de cases à cocher.
    const float menuWidth = ImGui::CalcTextSize("Moteur").x + ImGui::GetFrameHeight() * 1.5f;

    ImGui::SameLine();
    ImGui::SetNextItemWidth(menuWidth);
    RenderCategoryMenu("Moteur", Engine::LogSource::Engine);

    ImGui::SameLine();
    ImGui::SetNextItemWidth(menuWidth);
    RenderCategoryMenu("Jeu", Engine::LogSource::Game);
}

void ConsolePanel::SyncCategories()
{
    // Les catégories ne font que s'ajouter (le jeu peut en inventer une à tout moment),
    // il suffit donc de comparer leur nombre pour savoir s'il y a du nouveau.
    const auto &categories = Engine::Log::GetCategories();
    if (categories.size() == m_KnownCategoryCount)
        return;

    for (const Engine::LogCategoryInfo &category : categories)
        m_CategoryEnabled.emplace(category.Name, true);

    m_KnownCategoryCount = categories.size();
    m_FiltersDirty = true;
}

void ConsolePanel::RenderCategoryMenu(const char *label, Engine::LogSource source)
{
    if (!ImGui::BeginCombo(label, label, ImGuiComboFlags_NoPreview))
        return;

    bool any = false;
    for (const Engine::LogCategoryInfo &category : Engine::Log::GetCategories())
    {
        if (category.Source != source)
            continue;

        any = true;
        bool &enabled = m_CategoryEnabled[category.Name];
        if (ImGui::Checkbox(category.Name.c_str(), &enabled))
            m_FiltersDirty = true;
    }

    if (!any)
        ImGui::TextDisabled("Aucune catégorie");

    ImGui::EndCombo();
}

void ConsolePanel::RebuildVisibleLines()
{
    m_VisibleLines.clear();
    m_SeverityCounts[0] = m_SeverityCounts[1] = m_SeverityCounts[2] = 0;

    // Les compteurs des boutons portent sur tous les messages reçus, pas seulement sur
    // ceux affichés : c'est ce qui permet de savoir qu'il y a des erreurs alors même
    // qu'on les a masquées.
    const auto &messages = Engine::Log::GetConsoleMessages();

    // Index dans m_VisibleLines de la ligne déjà émise pour un texte donné (mode groupé).
    std::unordered_map<std::string, size_t> collapsedLines;

    for (const Engine::LogMessage &message : messages)
    {
        const int severity = (int)Engine::ToSeverityFilter(message.Level);
        ++m_SeverityCounts[severity];

        if (!m_ShowSeverity[severity])
            continue;

        auto category = m_CategoryEnabled.find(message.Category);
        if (category != m_CategoryEnabled.end() && !category->second)
            continue;

        if (m_Collapse)
        {
            auto existing = collapsedLines.find(message.Text);
            if (existing != collapsedLines.end())
            {
                ++m_VisibleLines[existing->second].Count;
                continue;
            }
            collapsedLines.emplace(message.Text, m_VisibleLines.size());
        }

        m_VisibleLines.push_back({message.Level, &message.Text, 1});
    }
}

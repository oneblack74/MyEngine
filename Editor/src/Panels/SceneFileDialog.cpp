#include "Panels/SceneFileDialog.h"
#include <cstdio>
#include <cstring>
#include <imgui.h>

namespace
{
    const char *TitleFor(SceneFileDialog::Mode mode)
    {
        return mode == SceneFileDialog::Mode::Open ? "Open Scene" : "Save Scene As";
    }

    // Le nom tapé à la main n'a pas forcément l'extension : l'ajouter évite de créer
    // un fichier que la boîte d'ouverture ne listerait plus ensuite.
    std::filesystem::path WithSceneExtension(const std::string &filename)
    {
        std::filesystem::path path = filename;
        if (path.extension() != Engine::k_SceneExtension)
            path += Engine::k_SceneExtension;
        return path;
    }
}

void SceneFileDialog::OpenDialog(Mode mode, const std::filesystem::path &directory,
                                 const std::string &suggestedName)
{
    m_Mode = mode;
    m_Directory = directory;
    m_Open = true;
    m_ShouldOpenPopup = true;

    std::snprintf(m_Filename, sizeof(m_Filename), "%s", suggestedName.c_str());
}

bool SceneFileDialog::OnImGuiRender(std::filesystem::path &outPath)
{
    if (!m_Open)
        return false;

    const char *title = TitleFor(m_Mode);

    // OpenPopup doit être appelé depuis la même pile d'ID que BeginPopupModal, donc ici
    // et pas dans OpenDialog (appelé depuis un menu, à un autre endroit de la pile).
    if (m_ShouldOpenPopup)
    {
        ImGui::OpenPopup(title);
        m_ShouldOpenPopup = false;
    }

    bool confirmed = false;

    // L'éditeur tourne avec ConfigViewportsNoAutoMerge (pour que la fenêtre "Game" soit
    // une vraie fenêtre OS) : sans ces deux lignes, la modale partirait elle aussi dans
    // sa propre fenêtre OS. On la garde centrée dans la fenêtre principale, comme une
    // boîte de dialogue normale.
    const ImGuiViewport *mainViewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowViewport(mainViewport->ID);
    ImGui::SetNextWindowPos(mainViewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(440.0f, 320.0f), ImGuiCond_Appearing);
    if (ImGui::BeginPopupModal(title, &m_Open))
    {
        // TextWrapped et pas TextDisabled : un chemin absolu dépasse largement la largeur
        // de la boîte et se ferait couper au milieu.
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        ImGui::TextWrapped("%s", m_Directory.string().c_str());
        ImGui::PopStyleColor();
        ImGui::Separator();

        // Deux lignes réservées en bas pour le champ de nom et les boutons.
        ImGui::BeginChild("Files", ImVec2(0.0f, -ImGui::GetFrameHeightWithSpacing() * 2.2f),
                          ImGuiChildFlags_Borders);
        bool empty = true;
        if (std::filesystem::exists(m_Directory))
        {
            for (const auto &entry : std::filesystem::directory_iterator(m_Directory))
            {
                if (!entry.is_regular_file() || entry.path().extension() != Engine::k_SceneExtension)
                    continue;

                empty = false;
                const std::string filename = entry.path().filename().string();
                if (ImGui::Selectable(filename.c_str(), filename == m_Filename))
                    std::snprintf(m_Filename, sizeof(m_Filename), "%s", filename.c_str());

                // Double-clic : sélectionner et valider d'un coup, comme partout ailleurs.
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                    confirmed = true;
            }
        }
        if (empty)
            ImGui::TextDisabled("No scene saved here yet");
        ImGui::EndChild();

        ImGui::SetNextItemWidth(-1.0f);
        // EnterReturnsTrue : taper un nom puis Entrée vaut un clic sur le bouton.
        if (ImGui::InputText("##Name", m_Filename, sizeof(m_Filename), ImGuiInputTextFlags_EnterReturnsTrue))
            confirmed = true;

        const bool hasName = m_Filename[0] != '\0';
        ImGui::BeginDisabled(!hasName);
        if (ImGui::Button(m_Mode == Mode::Open ? "Open" : "Save"))
            confirmed = true;
        ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            m_Open = false;
            ImGui::CloseCurrentPopup();
        }

        confirmed = confirmed && hasName;
        if (confirmed)
        {
            outPath = m_Directory / WithSceneExtension(m_Filename);
            m_Open = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    return confirmed;
}

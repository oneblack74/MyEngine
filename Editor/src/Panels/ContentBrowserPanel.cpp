#include "Panels/ContentBrowserPanel.h"
#include <Assets/AssetManager.h>
#include <string>
#include <imgui.h>

ContentBrowserPanel::ContentBrowserPanel()
    : m_RootDirectory(Engine::AssetManager::GetAssetRoot())
{
}

void ContentBrowserPanel::OnImGuiRender(const SceneActivatedCallback &onSceneActivated)
{
    m_OnSceneActivated = onSceneActivated;

    ImGui::Begin("Content Browser");

    if (std::filesystem::exists(m_RootDirectory))
        DrawDirectory(m_RootDirectory);
    else
        ImGui::TextDisabled("Asset folder not found: %s", m_RootDirectory.string().c_str());

    ImGui::End();
}

void ContentBrowserPanel::DrawDirectory(const std::filesystem::path &directory)
{
    for (auto &entry : std::filesystem::directory_iterator(directory))
    {
        const auto &path = entry.path();
        std::string filename = path.filename().string();

        if (entry.is_directory())
        {
            // Sans OpenOnArrow, toute la ligne ouvre le dossier et pas seulement la
            // flèche — c'est le comportement de VS Code, et ça évite de viser un
            // triangle de quelques pixels.
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
            bool open = ImGui::TreeNodeEx(filename.c_str(), flags);
            if (open)
            {
                DrawDirectory(path);
                ImGui::TreePop();
            }
        }
        else
        {
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen |
                                        ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_SpanAvailWidth;
            ImGui::TreeNodeEx(filename.c_str(), flags);

            // Double-clic sur une scène : l'ouvrir, comme dans n'importe quel éditeur.
            if (path.extension() == Engine::k_SceneExtension && m_OnSceneActivated &&
                ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                m_OnSceneActivated(path);
            }

            // Le fichier peut être déposé sur un champ d'asset de l'Inspecteur. La charge
            // utile est le chemin relatif à la racine des assets, c'est-à-dire exactement
            // ce qu'AssetManager::Import attend.
            if (ImGui::BeginDragDropSource())
            {
                const std::string relative =
                    std::filesystem::relative(path, m_RootDirectory).generic_string();

                // La taille inclut le zéro terminal : la cible reçoit une chaîne C utilisable telle quelle.
                ImGui::SetDragDropPayload(k_AssetPayloadType, relative.c_str(), relative.size() + 1);
                ImGui::TextUnformatted(filename.c_str());
                ImGui::EndDragDropSource();
            }
        }
    }
}

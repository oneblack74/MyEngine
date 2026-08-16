#include "Panels/ContentBrowserPanel.h"
#include <imgui.h>

ContentBrowserPanel::ContentBrowserPanel()
    : m_RootDirectory("assets")
{
}

void ContentBrowserPanel::OnImGuiRender()
{
    ImGui::Begin("Content Browser");

    if (std::filesystem::exists(m_RootDirectory))
        DrawDirectory(m_RootDirectory);
    else
        ImGui::TextDisabled("Dossier assets/ introuvable (cwd attendu : build/)");

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
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
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
        }
    }
}

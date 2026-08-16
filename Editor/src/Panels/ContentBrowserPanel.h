#pragma once
#include <filesystem>

// Arborescence de fichiers (façon VS Code), rootée sur le dossier assets/ du projet.
// Pas d'import/drag-drop pour l'instant : ça viendra avec le système d'assets (Phase 7).
class ContentBrowserPanel
{
public:
    ContentBrowserPanel();

    void OnImGuiRender();

private:
    void DrawDirectory(const std::filesystem::path &directory);

    std::filesystem::path m_RootDirectory;
};

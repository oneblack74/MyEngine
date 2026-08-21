#pragma once
#include <filesystem>

// Type de charge utile ImGui d'un asset glissé depuis le Content Browser : le chemin
// du fichier, relatif à la racine des assets. Partagé avec InspectorPanel, qui le reçoit.
inline constexpr const char *k_AssetPayloadType = "MYENGINE_ASSET";

// Arborescence de fichiers (façon VS Code), rootée sur le dossier des assets du projet.
// Chaque fichier peut être glissé vers un champ d'asset de l'Inspecteur.
class ContentBrowserPanel
{
public:
    ContentBrowserPanel();

    void OnImGuiRender();

private:
    void DrawDirectory(const std::filesystem::path &directory);

    std::filesystem::path m_RootDirectory;
};

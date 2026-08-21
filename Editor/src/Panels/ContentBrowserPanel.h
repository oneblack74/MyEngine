#pragma once
#include <Scene/SceneSerializer.h>
#include <filesystem>
#include <functional>

// Type de charge utile ImGui d'un asset glissé depuis le Content Browser : le chemin
// du fichier, relatif à la racine des assets. Partagé avec InspectorPanel, qui le reçoit.
inline constexpr const char *k_AssetPayloadType = "MYENGINE_ASSET";

// Arborescence de fichiers (façon VS Code), rootée sur le dossier des assets du projet.
// Chaque fichier peut être glissé vers un champ d'asset de l'Inspecteur.
class ContentBrowserPanel
{
public:
    ContentBrowserPanel();

    // onSceneActivated est appelée quand l'utilisateur double-clique un fichier de
    // scène. Le panel ne charge rien lui-même : c'est l'éditeur qui décide, et il a
    // besoin de le faire une fois tous les panels dessinés.
    using SceneActivatedCallback = std::function<void(const std::filesystem::path &)>;
    void OnImGuiRender(const SceneActivatedCallback &onSceneActivated);

private:
    void DrawDirectory(const std::filesystem::path &directory);

    SceneActivatedCallback m_OnSceneActivated;

    std::filesystem::path m_RootDirectory;
};

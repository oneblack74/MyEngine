#pragma once
#include <Scene/Scene.h>
#include <Scene/Entity.h>
#include <memory>
#include <string>

// Liste les entités de la scène active et gère la sélection (utilisée par InspectorPanel).
class SceneHierarchyPanel
{
public:
    SceneHierarchyPanel() = default;

    // Le nom est celui affiché en tête de la hiérarchie : le fichier de la scène, ou
    // "Untitled" tant qu'elle n'a jamais été enregistrée. Il est passé ici plutôt que
    // porté par Scene : c'est le chemin connu de l'éditeur qui le donne.
    void SetContext(const std::shared_ptr<Engine::Scene> &scene, const std::string &sceneName)
    {
        m_Context = scene;
        m_SceneName = sceneName;
    }
    void OnImGuiRender();

    Engine::Entity GetSelectedEntity() const { return m_SelectionContext; }
    void SetSelectedEntity(Engine::Entity entity) { m_SelectionContext = entity; }

private:
    void DrawEntityNode(Engine::Entity entity);

    std::shared_ptr<Engine::Scene> m_Context;
    std::string m_SceneName;
    Engine::Entity m_SelectionContext;
};

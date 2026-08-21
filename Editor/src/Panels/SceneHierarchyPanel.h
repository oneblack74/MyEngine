#pragma once
#include <Scene/Scene.h>
#include <Scene/Entity.h>
#include <memory>
#include <string>

// Type de charge utile ImGui d'une entité glissée dans la hiérarchie : son UUID.
inline constexpr const char *k_EntityPayloadType = "MYENGINE_ENTITY";

// Arbre des entités de la scène active. Gère la sélection (utilisée par InspectorPanel)
// et le glisser-déposer, qui sert autant à réordonner qu'à rattacher une entité à une
// autre.
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

    // Un dépôt de la frame écoulée. Le panel ne modifie pas la scène lui-même : le
    // faire au milieu du parcours de l'arbre reviendrait à le remanier pendant qu'on
    // le dessine, et l'éditeur veut de toute façon en faire une action annulable.
    struct HierarchyDrop
    {
        Engine::UUID Dragged{0};

        // Entité de référence. Nulle avec InsertBefore à false : dépôt sur la scène
        // elle-même, c'est-à-dire un retour à la racine.
        Engine::UUID Target{0};

        // true  : insérer juste avant Target, au même niveau qu'elle.
        // false : devenir le dernier enfant de Target.
        bool InsertBefore = false;
    };

    // Renvoie true et consomme le dépôt s'il y en a eu un cette frame.
    bool TakePendingDrop(HierarchyDrop &out);

private:
    void DrawEntityNode(Engine::Entity entity);

    std::shared_ptr<Engine::Scene> m_Context;
    std::string m_SceneName;
    Engine::Entity m_SelectionContext;

    bool m_HasPendingDrop = false;
    HierarchyDrop m_PendingDrop;
};

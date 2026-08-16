#pragma once
#include <Scene/Entity.h>

// Affiche et permet d'éditer les components de l'entité sélectionnée dans SceneHierarchyPanel.
class InspectorPanel
{
public:
    InspectorPanel() = default;

    void OnImGuiRender(Engine::Entity selectedEntity);

private:
    void DrawComponents(Engine::Entity entity);
};

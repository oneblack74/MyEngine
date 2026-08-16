#pragma once
#include <Scene/Scene.h>
#include <Scene/Entity.h>
#include <memory>

// Liste les entités de la scène active et gère la sélection (utilisée par InspectorPanel).
class SceneHierarchyPanel
{
public:
    SceneHierarchyPanel() = default;

    void SetContext(const std::shared_ptr<Engine::Scene> &scene) { m_Context = scene; }
    void OnImGuiRender();

    Engine::Entity GetSelectedEntity() const { return m_SelectionContext; }
    void SetSelectedEntity(Engine::Entity entity) { m_SelectionContext = entity; }

private:
    void DrawEntityNode(Engine::Entity entity);

    std::shared_ptr<Engine::Scene> m_Context;
    Engine::Entity m_SelectionContext;
};

#pragma once
#include <Renderer/Framebuffer.h>
#include <Renderer/OrthographicCamera.h>
#include <Scene/Scene.h>
#include <Scene/Entity.h>
#include <imgui.h> // doit être inclus avant ImGuizmo.h (qui utilise ImVec2/ImU32/... sans les inclure lui-même)
#include <ImGuizmo.h>
#include <glm/glm.hpp>
#include <functional>
#include <memory>

// Panel ImGui affichant la texture couleur d'un Framebuffer (la scène rendue hors écran),
// et suivant sa propre taille pour permettre à EditorLayer de redimensionner le Framebuffer.
// Dessine aussi le gizmo de transformation (ImGuizmo) sur l'entité sélectionnée, et gère
// la sélection d'entité au clic directement dans le viewport.
class ViewportPanel
{
public:
    ViewportPanel() = default;

    void OnImGuiRender(const std::shared_ptr<Engine::Framebuffer> &framebuffer,
                        const std::shared_ptr<Engine::Scene> &scene,
                        const Engine::OrthographicCamera &camera,
                        Engine::Entity selectedEntity,
                        const std::function<void(Engine::Entity)> &onEntityPicked);

    const glm::vec2 &GetSize() const { return m_Size; }
    bool IsFocused() const { return m_Focused; }

private:
    void DrawGizmo(const Engine::OrthographicCamera &camera, Engine::Entity selectedEntity);
    void HandlePicking(const std::shared_ptr<Engine::Scene> &scene, const Engine::OrthographicCamera &camera,
                        const glm::vec2 &imageScreenPos, const std::function<void(Engine::Entity)> &onEntityPicked);

    glm::vec2 m_Size = {0.0f, 0.0f};
    bool m_Focused = false;
    ImGuizmo::OPERATION m_GizmoOperation = ImGuizmo::TRANSLATE;
};

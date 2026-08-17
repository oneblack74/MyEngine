#include "Panels/ViewportPanel.h"
#include <Scene/Components.h>
#include <Core/Input.h>
#include <Core/KeyCodes.h>
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

static bool DecomposeTransform(const glm::mat4 &transform, glm::vec3 &translation, glm::vec3 &rotation, glm::vec3 &scale)
{
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::quat orientation;

    if (!glm::decompose(transform, scale, orientation, translation, skew, perspective))
        return false;

    rotation = glm::eulerAngles(orientation);
    return true;
}

void ViewportPanel::OnImGuiRender(const std::shared_ptr<Engine::Framebuffer> &framebuffer,
                                   const std::shared_ptr<Engine::Scene> &scene,
                                   Engine::OrthographicCamera &camera,
                                   float &cameraZoom,
                                   Engine::Entity selectedEntity,
                                   const std::function<void(Engine::Entity)> &onEntityPicked)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Viewport");

    m_Focused = ImGui::IsWindowFocused();
    bool hovered = ImGui::IsWindowHovered();

    ImVec2 panelSize = ImGui::GetContentRegionAvail();
    m_Size = {panelSize.x, panelSize.y};

    ImVec2 imageScreenPos = ImGui::GetCursorScreenPos();

    uint32_t textureID = framebuffer->GetColorAttachmentRendererID();
    ImGui::Image((ImTextureID)(intptr_t)textureID, ImVec2{m_Size.x, m_Size.y}, ImVec2{0, 1}, ImVec2{1, 0});

    HandleCameraNavigation(hovered, camera, cameraZoom);

    // Raccourcis Unity/Unreal : W = translate, E = rotate, R = scale.
    // On ignore pendant une manipulation en cours pour ne pas changer d'outil sous la souris.
    if (m_Focused && !ImGuizmo::IsUsing())
    {
        if (Engine::Input::IsKeyPressed(Engine::Key::W))
            m_GizmoOperation = ImGuizmo::TRANSLATE;
        else if (Engine::Input::IsKeyPressed(Engine::Key::E))
            m_GizmoOperation = ImGuizmo::ROTATE;
        else if (Engine::Input::IsKeyPressed(Engine::Key::R))
            m_GizmoOperation = ImGuizmo::SCALE;
    }

    DrawGizmo(camera, selectedEntity);

    // Le clic sélectionne une entité, sauf s'il atterrit sur le gizmo lui-même
    // (sinon on désélectionnerait/changerait de sélection en essayant de le manipuler).
    if (hovered && !ImGuizmo::IsOver() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        HandlePicking(scene, camera, {imageScreenPos.x, imageScreenPos.y}, onEntityPicked);

    ImGui::End();
    ImGui::PopStyleVar();
}

void ViewportPanel::DrawGizmo(const Engine::OrthographicCamera &camera, Engine::Entity selectedEntity)
{
    if (!selectedEntity || !selectedEntity.HasComponent<Engine::TransformComponent>())
        return;

    ImGuizmo::SetOrthographic(true);
    ImGuizmo::SetDrawlist();

    ImVec2 imageScreenPos = ImGui::GetItemRectMin();
    ImGuizmo::SetRect(imageScreenPos.x, imageScreenPos.y, m_Size.x, m_Size.y);

    auto &transformComponent = selectedEntity.GetComponent<Engine::TransformComponent>();
    glm::mat4 transform = transformComponent.GetTransform();

    ImGuizmo::Manipulate(glm::value_ptr(camera.GetViewMatrix()), glm::value_ptr(camera.GetProjectionMatrix()),
                          m_GizmoOperation, ImGuizmo::LOCAL, glm::value_ptr(transform));

    if (ImGuizmo::IsUsing())
    {
        glm::vec3 translation, rotation, scale;
        if (DecomposeTransform(transform, translation, rotation, scale))
        {
            // Moteur 2D : seule la rotation Z du TransformComponent est utilisée.
            transformComponent.Position = translation;
            transformComponent.Rotation = glm::degrees(rotation.z);
            transformComponent.Scale = scale;
        }
    }
}

void ViewportPanel::HandleCameraNavigation(bool hovered, Engine::OrthographicCamera &camera, float &cameraZoom)
{
    // Zoom (molette) : ne réagit que si la souris survole le viewport.
    if (hovered)
    {
        float scroll = ImGui::GetIO().MouseWheel;
        if (scroll != 0.0f)
            cameraZoom = glm::clamp(cameraZoom * (1.0f - scroll * 0.1f), 0.05f, 10.0f);
    }

    // Pan (clic molette + drag) : le drag démarre seulement si survolé, mais continue
    // même si la souris ressort du panel entre-temps (comportement standard d'éditeur).
    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
        m_IsPanning = true;
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Middle))
        m_IsPanning = false;

    if (m_IsPanning && m_Size.y > 0.0f)
    {
        // Convertit un delta écran (pixels) en delta monde, selon la portion de scène
        // actuellement visible dans le viewport (dépend du zoom courant).
        float aspectRatio = m_Size.x / m_Size.y;
        float worldWidth = 2.0f * aspectRatio * cameraZoom;
        float worldHeight = 2.0f * cameraZoom;

        ImVec2 delta = ImGui::GetIO().MouseDelta;
        glm::vec3 position = camera.GetPosition();
        position.x -= (delta.x / m_Size.x) * worldWidth;
        position.y += (delta.y / m_Size.y) * worldHeight; // écran Y vers le bas, monde Y vers le haut
        camera.SetPosition(position);
    }
}

void ViewportPanel::HandlePicking(const std::shared_ptr<Engine::Scene> &scene, const Engine::OrthographicCamera &camera,
                                   const glm::vec2 &imageScreenPos, const std::function<void(Engine::Entity)> &onEntityPicked)
{
    if (!scene || m_Size.x <= 0.0f || m_Size.y <= 0.0f)
        return;

    ImVec2 mousePos = ImGui::GetMousePos();
    glm::vec2 mouseInImage = {mousePos.x - imageScreenPos.x, mousePos.y - imageScreenPos.y};

    // Position de la souris -> NDC -> espace monde (via l'inverse de la view-projection)
    glm::vec2 ndc = {
        (mouseInImage.x / m_Size.x) * 2.0f - 1.0f,
        1.0f - (mouseInImage.y / m_Size.y) * 2.0f,
    };
    glm::mat4 invViewProjection = glm::inverse(camera.GetViewProjectionMatrix());
    glm::vec4 worldPoint = invViewProjection * glm::vec4(ndc, 0.0f, 1.0f);

    Engine::Entity picked;
    auto view = scene->GetAllEntitiesWith<Engine::TransformComponent>();
    for (auto entityHandle : view)
    {
        Engine::Entity entity{entityHandle, scene.get()};
        auto &transform = entity.GetComponent<Engine::TransformComponent>();

        // Point testé en espace local du quad (qui s'étend de -0.5 à 0.5 avant transform)
        glm::vec4 localPoint = glm::inverse(transform.GetTransform()) * glm::vec4(worldPoint.x, worldPoint.y, 0.0f, 1.0f);

        if (localPoint.x >= -0.5f && localPoint.x <= 0.5f && localPoint.y >= -0.5f && localPoint.y <= 0.5f)
            picked = entity; // le dernier hit gagne (dessiné par-dessus les précédents)
    }

    if (onEntityPicked)
        onEntityPicked(picked);
}

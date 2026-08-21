#include "Panels/GamePanel.h"
#include <Renderer/Renderer.h>
#include <Scene/Entity.h>
#include <Scene/Components.h>
#include <Scene/RenderSystem.h>
#include <imgui.h>

static Engine::Entity FindPrimaryCamera(Engine::Scene &scene)
{
    auto view = scene.GetAllEntitiesWith<Engine::CameraComponent>();
    for (auto entityHandle : view)
    {
        Engine::Entity entity{entityHandle, &scene};
        if (entity.GetComponent<Engine::CameraComponent>().Primary)
            return entity;
    }
    return {};
}

GamePanel::GamePanel()
{
    Engine::FramebufferSpecification spec;
    spec.Width = (uint32_t)m_TargetWidth;
    spec.Height = (uint32_t)m_TargetHeight;
    m_Framebuffer = std::make_shared<Engine::Framebuffer>(spec);
}

bool GamePanel::OnImGuiRender(const std::shared_ptr<Engine::Scene> &scene, bool visible)
{
    if (!visible)
        return false;

    // Toujours flottante, jamais docké dans le layout principal — combiné à
    // ConfigViewportsNoAutoMerge (réglé une fois dans EditorLayer::OnAttach), ça force
    // une vraie fenêtre OS séparée à chaque ouverture, plutôt qu'un onglet du dock.
    ImGui::SetNextWindowSize(ImVec2(960.0f, 620.0f), ImGuiCond_Appearing);
    const ImGuiViewport *mainViewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(mainViewport->Pos.x + 60.0f, mainViewport->Pos.y + 60.0f), ImGuiCond_Appearing);

    bool open = true;
    ImGui::Begin("Game", &open, ImGuiWindowFlags_NoDocking);

    ImGui::SetNextItemWidth(200.0f);
    ImGui::SliderFloat("Scale", &m_Scale, 0.1f, 2.0f, "%.2fx");
    ImGui::SameLine();
    ImGui::Text("(%dx%d)", m_TargetWidth, m_TargetHeight);

    Engine::Entity primaryCamera;
    if (scene)
        primaryCamera = FindPrimaryCamera(*scene);

    if (!primaryCamera)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f),
                            "No primary camera (a CameraComponent with Primary) in the scene");
        ImGui::End();
        return open;
    }

    auto &cameraComponent = primaryCamera.GetComponent<Engine::CameraComponent>();
    auto &transform = primaryCamera.GetComponent<Engine::TransformComponent>();

    // La projection suit toujours le ratio de la résolution cible (jamais celui de la
    // fenêtre ImGui, qui n'affiche l'image qu'à l'échelle du slider) ; la position/
    // rotation viennent du TransformComponent, pas d'un état propre à la caméra.
    float aspectRatio = (float)m_TargetWidth / (float)m_TargetHeight;
    float size = cameraComponent.OrthographicSize;
    cameraComponent.Camera.SetProjection(-aspectRatio * size, aspectRatio * size, -size, size);
    cameraComponent.Camera.SetPosition(transform.Position);
    cameraComponent.Camera.SetRotation(transform.Rotation);

    m_Framebuffer->Bind();
    Engine::Renderer::SetClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    Engine::Renderer::Clear();
    Engine::RenderSystem::Render(*scene, cameraComponent.Camera);
    m_Framebuffer->Unbind();

    // Rendu à résolution fixe, affiché à l'échelle du slider : si l'image dépasse la
    // zone visible (scale > 1, ou fenêtre petite), on scrolle au lieu d'écraser le ratio.
    ImGui::BeginChild("GameRenderArea", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_HorizontalScrollbar);
    ImVec2 imageSize(m_TargetWidth * m_Scale, m_TargetHeight * m_Scale);
    uint32_t textureID = m_Framebuffer->GetColorAttachmentRendererID();
    ImGui::Image((ImTextureID)(intptr_t)textureID, imageSize, ImVec2{0, 1}, ImVec2{1, 0});
    ImGui::EndChild();

    ImGui::End();

    return open;
}

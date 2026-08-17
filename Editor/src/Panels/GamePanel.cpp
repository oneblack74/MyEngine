#include "Panels/GamePanel.h"
#include <Renderer/Renderer.h>
#include <Scene/RenderSystem.h>
#include <imgui.h>

GamePanel::GamePanel() : m_Camera(-1.6f, 1.6f, -0.9f, 0.9f)
{
    Engine::FramebufferSpecification spec;
    spec.Width = (uint32_t)m_TargetWidth;
    spec.Height = (uint32_t)m_TargetHeight;
    m_Framebuffer = std::make_shared<Engine::Framebuffer>(spec);

    float aspectRatio = (float)m_TargetWidth / (float)m_TargetHeight;
    constexpr float zoom = 0.9f;
    m_Camera.SetProjection(-aspectRatio * zoom, aspectRatio * zoom, -zoom, zoom);
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

    if (scene)
    {
        m_Framebuffer->Bind();
        Engine::Renderer::SetClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        Engine::Renderer::Clear();
        Engine::RenderSystem::Render(*scene, m_Camera);
        m_Framebuffer->Unbind();
    }

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

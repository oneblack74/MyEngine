#include "Panels/ViewportPanel.h"
#include <imgui.h>

void ViewportPanel::OnImGuiRender(const std::shared_ptr<Engine::Framebuffer> &framebuffer)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Viewport");

    m_Focused = ImGui::IsWindowFocused();

    ImVec2 panelSize = ImGui::GetContentRegionAvail();
    m_Size = {panelSize.x, panelSize.y};

    uint32_t textureID = framebuffer->GetColorAttachmentRendererID();
    ImGui::Image((ImTextureID)(intptr_t)textureID, ImVec2{m_Size.x, m_Size.y}, ImVec2{0, 1}, ImVec2{1, 0});

    ImGui::End();
    ImGui::PopStyleVar();
}

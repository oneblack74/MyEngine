#pragma once
#include <Renderer/Framebuffer.h>
#include <glm/glm.hpp>
#include <memory>

// Panel ImGui affichant la texture couleur d'un Framebuffer (la scène rendue hors écran),
// et suivant sa propre taille pour permettre à EditorLayer de redimensionner le Framebuffer.
class ViewportPanel
{
public:
    ViewportPanel() = default;

    void OnImGuiRender(const std::shared_ptr<Engine::Framebuffer> &framebuffer);

    const glm::vec2 &GetSize() const { return m_Size; }
    bool IsFocused() const { return m_Focused; }

private:
    glm::vec2 m_Size = {0.0f, 0.0f};
    bool m_Focused = false;
};

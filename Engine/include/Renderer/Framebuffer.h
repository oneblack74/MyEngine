#pragma once
#include <cstdint>

namespace Engine
{
    struct FramebufferSpecification
    {
        uint32_t Width = 0, Height = 0;
    };

    // Render target hors écran (FBO) : la scène est dessinée dans une texture
    // au lieu de l'écran, pour pouvoir l'afficher dans un panel ImGui (ViewportPanel).
    class Framebuffer
    {
    public:
        Framebuffer(const FramebufferSpecification &spec);
        ~Framebuffer();

        void Bind() const;
        void Unbind() const;

        void Resize(uint32_t width, uint32_t height);

        uint32_t GetColorAttachmentRendererID() const { return m_ColorAttachment; }
        const FramebufferSpecification &GetSpecification() const { return m_Specification; }

    private:
        void Invalidate();

        uint32_t m_RendererID = 0;
        uint32_t m_ColorAttachment = 0, m_DepthAttachment = 0;
        FramebufferSpecification m_Specification;
    };
}

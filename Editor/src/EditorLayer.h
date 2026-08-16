#pragma once
#include "Panels/ViewportPanel.h"
#include <Core/Layer.h>
#include <Renderer/Framebuffer.h>
#include <Renderer/OrthographicCamera.h>
#include <Scene/Scene.h>
#include <memory>

class EditorLayer : public Engine::Layer
{
public:
    EditorLayer();

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate() override;

private:
    void RenderImGui();

    std::shared_ptr<Engine::Framebuffer> m_Framebuffer;
    std::shared_ptr<Engine::OrthographicCamera> m_Camera;
    std::shared_ptr<Engine::Scene> m_ActiveScene;

    ViewportPanel m_ViewportPanel;
};

#pragma once
#include "Panels/ViewportPanel.h"
#include "Panels/SceneHierarchyPanel.h"
#include "Panels/InspectorPanel.h"
#include "Panels/ContentBrowserPanel.h"
#include "Panels/ConsolePanel.h"
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
    SceneHierarchyPanel m_SceneHierarchyPanel;
    InspectorPanel m_InspectorPanel;
    ContentBrowserPanel m_ContentBrowserPanel;
    ConsolePanel m_ConsolePanel;
};

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

enum class SceneState
{
    Edit = 0,
    Play = 1
};

class EditorLayer : public Engine::Layer
{
public:
    EditorLayer();

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate() override;

private:
    void RenderImGui();
    void SetupDefaultDockLayout();

    void OnScenePlay();
    void OnSceneStop();
    const std::shared_ptr<Engine::Scene> &GetActiveScene() const
    {
        return m_SceneState == SceneState::Edit ? m_EditorScene : m_RuntimeScene;
    }

    std::shared_ptr<Engine::Framebuffer> m_Framebuffer;
    std::shared_ptr<Engine::OrthographicCamera> m_Camera;

    // m_EditorScene est la scène éditée, jamais modifiée pendant le Play.
    // m_RuntimeScene est une copie créée au Play et jetée au Stop.
    std::shared_ptr<Engine::Scene> m_EditorScene;
    std::shared_ptr<Engine::Scene> m_RuntimeScene;
    SceneState m_SceneState = SceneState::Edit;
    bool m_ScenePaused = false;

    ViewportPanel m_ViewportPanel;
    SceneHierarchyPanel m_SceneHierarchyPanel;
    InspectorPanel m_InspectorPanel;
    ContentBrowserPanel m_ContentBrowserPanel;
    ConsolePanel m_ConsolePanel;

    bool m_ResetDockLayoutRequested = false;
    bool m_LoadLastSavedLayoutRequested = false;
};

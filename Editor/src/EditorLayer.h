#pragma once
#include "Panels/ViewportPanel.h"
#include "Testing/EditorTestOptions.h"
#include "Panels/GamePanel.h"
#include "Panels/SceneHierarchyPanel.h"
#include "Panels/InspectorPanel.h"
#include "Panels/ContentBrowserPanel.h"
#include "Panels/ConsolePanel.h"
#include <Core/Layer.h>
#include <Renderer/Framebuffer.h>
#include <Renderer/OrthographicCamera.h>
#include <Scene/Scene.h>
#include <Scene/PhysicsSystem.h>
#include <memory>

enum class SceneState
{
    Edit = 0,
    Play = 1
};

class EditorLayer : public Engine::Layer
{
public:
    explicit EditorLayer(const EditorTestOptions &testOptions = {});

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(Engine::Timestep ts) override;

private:
    void RenderImGui();
    void RunTestModeStep();
    void SetupDefaultDockLayout();

    void OnScenePlay();
    void OnSceneStop();
    const std::shared_ptr<Engine::Scene> &GetActiveScene() const
    {
        return m_SceneState == SceneState::Edit ? m_EditorScene : m_RuntimeScene;
    }

    std::shared_ptr<Engine::Framebuffer> m_Framebuffer;
    std::shared_ptr<Engine::OrthographicCamera> m_Camera;
    float m_CameraZoom = 0.9f; // demi-hauteur visible en unités monde ; ajusté à la molette dans le Viewport

    // m_EditorScene est la scène éditée, jamais modifiée pendant le Play.
    // m_RuntimeScene est une copie créée au Play et jetée au Stop.
    std::shared_ptr<Engine::Scene> m_EditorScene;
    std::shared_ptr<Engine::Scene> m_RuntimeScene;
    SceneState m_SceneState = SceneState::Edit;
    bool m_ScenePaused = false;

    Engine::PhysicsSystem m_PhysicsSystem;

    ViewportPanel m_ViewportPanel;
    GamePanel m_GamePanel;
    SceneHierarchyPanel m_SceneHierarchyPanel;
    InspectorPanel m_InspectorPanel;
    ContentBrowserPanel m_ContentBrowserPanel;
    ConsolePanel m_ConsolePanel;

    bool m_ResetDockLayoutRequested = false;
    bool m_LoadLastSavedLayoutRequested = false;

    // Mode test : inactif tant qu'aucun argument de ligne de commande ne l'active.
    EditorTestOptions m_TestOptions;
    int m_FrameCount = 0;
};

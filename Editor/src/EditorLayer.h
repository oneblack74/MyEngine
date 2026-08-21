#pragma once
#include "Panels/ViewportPanel.h"
#include "Commands/CommandHistory.h"
#include "Commands/EditorContext.h"
#include "Testing/EditorTestEngine.h"
#include "Panels/GamePanel.h"
#include "Panels/SceneHierarchyPanel.h"
#include "Panels/InspectorPanel.h"
#include "Panels/ContentBrowserPanel.h"
#include "Panels/ConsolePanel.h"
#include "Panels/SceneFileDialog.h"
#include <Core/Layer.h>
#include <Renderer/Framebuffer.h>
#include <Renderer/OrthographicCamera.h>
#include <Scene/Scene.h>
#include <Scene/AudioSystem.h>
#include <Scene/PhysicsSystem.h>
#include <filesystem>
#include <memory>
#include <string>

enum class SceneState
{
    Edit = 0,
    Play = 1
};

class EditorLayer : public Engine::Layer, public EditorContext
{
public:
    explicit EditorLayer(const EditorTestOptions &testOptions = {});

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(Engine::Timestep ts) override;

    // EditorContext — ce que les commandes d'annulation voient de l'éditeur.
    Engine::Scene &GetEditorScene() override { return *m_EditorScene; }
    void SelectEntity(Engine::Entity entity) override { m_SceneHierarchyPanel.SetSelectedEntity(entity); }

    CommandHistory &GetCommandHistoryForTests() { return m_CommandHistory; }
    bool AreColliderOutlinesVisibleForTests() const { return m_ShowColliderOutlines; }
    ConsolePanel &GetConsolePanelForTests() { return m_ConsolePanel; }

    // Code de sortie du processus après une exécution de tests (0 si tout va bien).
    int GetTestExitCode() const { return m_TestEngine.ExitCode(); }

    // Accès réservé aux tests automatisés (Testing/EditorTests.cpp) : ils pilotent l'UI
    // puis vérifient l'état réel de l'éditeur derrière.
    Engine::Entity GetSelectedEntityForTests() const { return m_SceneHierarchyPanel.GetSelectedEntity(); }
    bool IsPlayingForTests() const { return m_SceneState == SceneState::Play; }
    const std::filesystem::path &GetCurrentScenePathForTests() const { return m_CurrentScenePath; }

private:
    void RenderImGui();
    void RunTestModeStep();
    void HandleShortcuts();
    void TrackGizmoEdit();
    void RenderFileMenu();
    void RenderEditMenu();
    void RenderViewMenu();

    // Actions du menu Fichier.
    void NewScene();
    void OpenScene(const std::filesystem::path &path);
    void SaveScene();   // Ctrl+S : réécrit la scène courante, ou demande où la mettre
    void SaveSceneAs(); // ouvre la boîte de dialogue, même si la scène a déjà un chemin
    void SaveSceneTo(const std::filesystem::path &path);
    void ShowSceneDialog(SceneFileDialog::Mode mode);

    // Installe une scène comme scène d'édition et note d'où elle vient (chemin vide
    // pour une scène jamais enregistrée).
    void SetEditorScene(const std::shared_ptr<Engine::Scene> &scene, const std::filesystem::path &path);
    void UpdateWindowTitle();

    // Nom de la scène tel qu'affiché (titre de la fenêtre, tête de la hiérarchie).
    std::string GetSceneDisplayName() const;

    // Dossier des scènes du projet, sous la racine des assets.
    static std::filesystem::path GetSceneDirectory();

    // Actions du menu Édition, chacune passant par une commande annulable.
    void CopySelectedEntity();
    void PasteEntity();
    void DuplicateSelectedEntity();
    void DeleteSelectedEntity();
    void SetupDefaultDockLayout();

    // Applique un glisser-déposer de la hiérarchie, en passant par l'historique pour
    // que le déplacement soit annulable comme le reste.
    void ApplyHierarchyDrop(const SceneHierarchyPanel::HierarchyDrop &drop);
    void InstantiateScene(const SceneHierarchyPanel::SceneInstanceDrop &drop);

    // Répercute une scène source modifiée sur toutes ses instances de la scène éditée.
    void RefreshSceneInstances(Engine::AssetHandle source);

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

    CommandHistory m_CommandHistory;

    // Presse-papiers : une scène détachée qui ne sert qu'à garder en vie une copie de
    // l'entité copiée, y compris après sa suppression.
    std::shared_ptr<Engine::Scene> m_Clipboard;
    Engine::Entity m_ClipboardEntity;

    // Suivi du drag de gizmo : ImGuizmo écrit dans le Transform à chaque frame, la
    // commande d'annulation n'est empilée qu'au relâchement, pour qu'un déplacement
    // entier ne compte que pour une seule annulation.
    bool m_GizmoWasUsing = false;
    Engine::UUID m_GizmoEntityID{0};
    Engine::TransformComponent m_TransformBeforeGizmo;

    // Chemin de la scène ouverte, vide tant qu'elle n'a jamais été enregistrée :
    // c'est ce qui permet à Ctrl+S de réécrire au même endroit sans rien demander.
    std::filesystem::path m_CurrentScenePath;
    SceneFileDialog m_SceneFileDialog;

    // Scène demandée par un double-clic dans le Content Browser. Le chargement est
    // repoussé après le dessin de tous les panels : changer de scène au milieu d'une
    // frame remplacerait celle que les panels déjà dessinés viennent d'utiliser.
    std::filesystem::path m_SceneToOpen;

    ViewportPanel m_ViewportPanel;
    GamePanel m_GamePanel;
    SceneHierarchyPanel m_SceneHierarchyPanel;
    InspectorPanel m_InspectorPanel;
    ContentBrowserPanel m_ContentBrowserPanel;
    ConsolePanel m_ConsolePanel;

    // Le disque n'est interrogé que quelques fois par seconde : vérifier la date de
    // chaque asset à chaque frame ne servirait qu'à marteler le système de fichiers.
    float m_AssetReloadTimer = 0.0f;

    // Contours des colliders superposés au Viewport (menu Affichage).
    bool m_ShowColliderOutlines = false;

    bool m_ResetDockLayoutRequested = false;
    bool m_LoadLastSavedLayoutRequested = false;

    // Mode test : inactif tant qu'aucun argument de ligne de commande ne l'active.
    EditorTestOptions m_TestOptions;
    EditorTestEngine m_TestEngine;
    int m_FrameCount = 0;
};

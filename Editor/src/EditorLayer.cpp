#include "EditorLayer.h"
#include "Commands/SceneCommands.h"
#include <Core/Log.h>
#include <Core/Application.h>
#include <Utils/ImageCapture.h>
#include <Renderer/Renderer.h>
#include <Renderer/Renderer2D.h>
#include <Scene/Entity.h>
#include <Scene/Components.h>
#include <Scene/RenderSystem.h>
#include <Scene/SceneManager.h>
#include <imgui.h>
#include <imgui_internal.h> // DockBuilder* : API "interne" ImGui, mais c'est le seul moyen de définir un layout par défaut
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <ImGuizmo.h>
#include <GLFW/glfw3.h>

EditorLayer::EditorLayer(const EditorTestOptions &testOptions)
    : Layer("EditorLayer"), m_TestOptions(testOptions) {}

void EditorLayer::OnAttach()
{
    Engine::Renderer2D::Init();

    Engine::FramebufferSpecification spec;
    spec.Width = 1280;
    spec.Height = 720;
    m_Framebuffer = std::make_shared<Engine::Framebuffer>(spec);

    m_Camera = std::make_shared<Engine::OrthographicCamera>(-1.6f, 1.6f, -0.9f, 0.9f);

    // Scène de démo, le temps que SceneHierarchyPanel/ContentBrowserPanel existent
    m_EditorScene = Engine::SceneManager::NewScene();
    auto square = m_EditorScene->CreateEntity("Square");
    auto &transform = square.GetComponent<Engine::TransformComponent>();
    transform.Scale = {0.5f, 0.5f, 1.0f};
    square.AddComponent<Engine::SpriteRendererComponent>(glm::vec4(0.2f, 0.6f, 0.9f, 1.0f));

    // Démo Phase 5 : au Play, ce carré doit tomber sous l'effet de la gravité et
    // s'arrêter sur le sol statique — à retirer une fois PhysicsSystem validé visuellement.
    auto &squareBody = square.AddComponent<Engine::RigidBodyComponent>();
    squareBody.Type = Engine::RigidBodyComponent::BodyType::Dynamic;
    square.AddComponent<Engine::BoxColliderComponent>();

    // Même démo, avec un CircleColliderComponent cette fois — le rendu est identique
    // (Renderer2D ne dessine que des quads pour l'instant, pas de vrai rendu de cercle),
    // donc ça ne se verra pas à l'écran : sert juste à vérifier que la shape circulaire
    // se crée et simule sans crash.
    auto circle = m_EditorScene->CreateEntity("Circle");
    auto &circleTransform = circle.GetComponent<Engine::TransformComponent>();
    circleTransform.Position = {0.7f, 0.3f, 0.0f};
    circleTransform.Scale = {0.4f, 0.4f, 1.0f};
    circle.AddComponent<Engine::SpriteRendererComponent>(glm::vec4(0.9f, 0.5f, 0.2f, 1.0f));
    auto &circleBody = circle.AddComponent<Engine::RigidBodyComponent>();
    circleBody.Type = Engine::RigidBodyComponent::BodyType::Dynamic;
    circle.AddComponent<Engine::CircleColliderComponent>();

    auto ground = m_EditorScene->CreateEntity("Ground");
    auto &groundTransform = ground.GetComponent<Engine::TransformComponent>();
    groundTransform.Position = {0.0f, -0.7f, 0.0f};
    groundTransform.Scale = {2.5f, 0.2f, 1.0f};
    ground.AddComponent<Engine::SpriteRendererComponent>(glm::vec4(0.5f, 0.4f, 0.3f, 1.0f));
    ground.AddComponent<Engine::RigidBodyComponent>();
    ground.AddComponent<Engine::BoxColliderComponent>();

    // Démo CameraComponent : sans ça, GamePanel n'a aucune caméra "Primary" à utiliser
    // et affiche juste son avertissement à la place d'un rendu.
    auto mainCamera = m_EditorScene->CreateEntity("Main Camera");
    mainCamera.AddComponent<Engine::CameraComponent>();

    // Démo Phase 5 : logue chaque début/fin de collision dans la Console pour vérifier
    // que les contact events Box2D remontent bien jusqu'à l'ECS (via Entity, pas juste
    // des b2ShapeId bruts).
    m_PhysicsSystem.OnCollisionBegin = [](Engine::Entity a, Engine::Entity b)
    { LOG_INFO("Collision : {0} <-> {1}", a.GetName(), b.GetName()); };
    m_PhysicsSystem.OnCollisionEnd = [](Engine::Entity a, Engine::Entity b)
    { LOG_INFO("Fin de collision : {0} <-> {1}", a.GetName(), b.GetName()); };

    m_SceneHierarchyPanel.SetContext(m_EditorScene);

    // Init ImGui (contexte + backends GLFW/OpenGL3), avec le docking activé
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    // Multi-viewport : permet à une fenêtre flottante (comme "Game") de devenir une vraie
    // fenêtre OS. NoAutoMerge empêche ImGui de la refondre dans la fenêtre principale
    // quand elle apparaît par-dessus/à côté — sans ça, "Game" ne deviendrait une fenêtre
    // séparée que si on la faisait glisser assez loin manuellement.
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.ConfigViewportsNoAutoMerge = true;
    // Un simple clic-relâché (sans bouger la souris) sur un DragFloat le transforme en
    // champ de saisie texte : on peut donc taper une valeur au clavier dans l'Inspecteur
    // au lieu de devoir la faire glisser. Le drag reste inchangé dès que la souris bouge.
    io.ConfigDragClickToInputText = true;

    ImGui::StyleColorsDark();

    // Sans ça, les fenêtres OS détachées ont un fond transparent et des coins arrondis
    // qui ne collent pas avec le style de la fenêtre principale (recette standard ImGui
    // pour ImGuiConfigFlags_ViewportsEnable).
    ImGuiStyle &style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    GLFWwindow *window = Engine::Application::Get().GetWindow().GetNativeWindow();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Après la création du contexte ImGui : le Test Engine s'y accroche.
    m_InspectorPanel.SetEditorContext(this);
    m_TestEngine.Start(*this, m_TestOptions);
}

void EditorLayer::OnDetach()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // Après ImGui::DestroyContext() : le Test Engine sauve ses propres réglages à ce
    // moment-là et a besoin que le contexte ImGui soit déjà parti.
    m_TestEngine.Stop();

    Engine::Renderer2D::Shutdown();
}

void EditorLayer::OnUpdate(Engine::Timestep ts)
{
    // Le swap de la frame précédente vient d'avoir lieu dans Window::OnUpdate : c'est
    // le moment attendu par le Test Engine pour son PostSwap.
    m_TestEngine.OnFrameStart();

    // La physique ne tourne que pendant le Play, et se fige en Pause (comme le
    // ferait n'importe quel moteur : Pause gèle la simulation, pas juste le rendu).
    if (m_SceneState == SceneState::Play && !m_ScenePaused)
        m_PhysicsSystem.OnUpdate(*m_RuntimeScene, ts);

    // Redimensionne le Framebuffer si le panel Viewport a changé de taille
    const glm::vec2 &viewportSize = m_ViewportPanel.GetSize();
    const auto &spec = m_Framebuffer->GetSpecification();
    if (viewportSize.x > 0.0f && viewportSize.y > 0.0f &&
        (spec.Width != (uint32_t)viewportSize.x || spec.Height != (uint32_t)viewportSize.y))
    {
        m_Framebuffer->Resize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
    }

    // La projection est recalculée à chaque frame (pas seulement au resize) : elle doit
    // suivre le ratio largeur/hauteur du panel (sinon l'image est étirée) ET le zoom
    // courant, modifié à la molette dans le Viewport la frame précédente.
    if (spec.Width > 0 && spec.Height > 0)
    {
        float aspectRatio = (float)spec.Width / (float)spec.Height;
        m_Camera->SetProjection(-aspectRatio * m_CameraZoom, aspectRatio * m_CameraZoom, -m_CameraZoom, m_CameraZoom);
    }

    // Rendu de la scène hors écran, dans le Framebuffer
    m_Framebuffer->Bind();
    Engine::Renderer::SetClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    Engine::Renderer::Clear();
    Engine::RenderSystem::Render(*GetActiveScene(), *m_Camera);
    if (m_ShowColliderOutlines)
        Engine::RenderSystem::RenderColliderOutlines(*GetActiveScene(), *m_Camera);
    m_Framebuffer->Unbind();

    // Rendu de l'UI ImGui (dockspace + panels) par-dessus
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame(); // requis une fois par frame pour qu'ImGuizmo suive le drag de la souris

    RenderImGui();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Dessine les fenêtres OS détachées (panels sortis de la fenêtre principale) — no-op
    // tant qu'aucun panel n'a été glissé dehors. Doit rester le dernier appel ImGui de la
    // frame, et on restaure le contexte GL courant après car ça en change pendant l'appel.
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow *backupContext = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backupContext);
    }

    RunTestModeStep();
}

void EditorLayer::RunTestModeStep()
{
    if (m_TestOptions.ScreenshotPath.empty())
        return;

    // La capture doit avoir lieu ici, à la fin de la frame et avant le swap fait par
    // Window::OnUpdate : une fois le swap passé, GL_BACK ne contient plus ce qu'on
    // vient de dessiner.
    if (++m_FrameCount < m_TestOptions.WarmupFrames)
        return;

    // Taille du framebuffer et pas celle de la fenêtre : les deux diffèrent sur un
    // écran à mise à l'échelle (HiDPI), et glReadPixels raisonne en pixels réels.
    GLFWwindow *window = Engine::Application::Get().GetWindow().GetNativeWindow();
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);

    const bool ok = Engine::ImageCapture::CaptureBackBuffer(m_TestOptions.ScreenshotPath,
                                                            (uint32_t)width, (uint32_t)height);
    if (!ok)
        LOG_ERROR("Mode test : la capture a échoué");

    Engine::Application::Get().Close();
}

void EditorLayer::RenderImGui()
{
    HandleShortcuts();

    // Dockspace plein écran — recette standard de la démo ImGui (docking branch)
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace", nullptr, windowFlags);
    ImGui::PopStyleVar(3);

    ImGuiID dockspaceId = ImGui::GetID("MyEngineDockSpace");

    // La reconstruction du dock tree (DockBuilder*) doit se faire AVANT le DockSpace()
    // de cette frame, jamais pendant/après (ça corromprait l'état actif en cours de frame,
    // les fenêtres perdent leur ancrage) — donc on la déclenche ici, résultat d'un clic
    // menu de la frame précédente, ou parce qu'aucune disposition n'a été chargée depuis
    // imgui.ini (premier lancement, build/ tout neuf...).
    if (m_ResetDockLayoutRequested || ImGui::DockBuilderGetNode(dockspaceId) == nullptr)
    {
        SetupDefaultDockLayout();
        m_ResetDockLayoutRequested = false;
    }

    // Même règle : recharger l'ini doit aussi se faire avant DockSpace(), pas pendant.
    if (m_LoadLastSavedLayoutRequested)
    {
        ImGui::LoadIniSettingsFromDisk(ImGui::GetIO().IniFilename);
        m_LoadLastSavedLayoutRequested = false;
    }

    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f));

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("Fichier"))
        {
            ImGui::MenuItem("Quitter (bientôt)", nullptr, false, false);
            ImGui::EndMenu();
        }
        RenderEditMenu();
        RenderViewMenu();
        if (ImGui::BeginMenu("Fenêtre"))
        {
            if (ImGui::MenuItem("Réinitialiser la disposition"))
                m_ResetDockLayoutRequested = true;
            if (ImGui::MenuItem("Sauvegarder la disposition"))
            {
                ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
                LOG_INFO("Disposition des panels sauvegardée");
            }
            if (ImGui::MenuItem("Charger la dernière disposition sauvegardée"))
                m_LoadLastSavedLayoutRequested = true;
            ImGui::EndMenu();
        }

        // Toolbar Play/Pause/Stop, centrée dans la barre de menu
        float buttonsWidth = m_SceneState == SceneState::Edit ? 50.0f : 120.0f;
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() * 0.5f - buttonsWidth * 0.5f);
        if (m_SceneState == SceneState::Edit)
        {
            if (ImGui::Button("Play"))
                OnScenePlay();
        }
        else
        {
            if (ImGui::Button(m_ScenePaused ? "Reprendre" : "Pause"))
                m_ScenePaused = !m_ScenePaused;
            ImGui::SameLine();
            if (ImGui::Button("Stop"))
                OnSceneStop();
        }

        ImGui::EndMenuBar();
    }

    ImGui::End();

    m_ViewportPanel.OnImGuiRender(m_Framebuffer, GetActiveScene(), *m_Camera, m_CameraZoom,
                                   m_SceneHierarchyPanel.GetSelectedEntity(),
                                   [this](Engine::Entity picked)
                                   { m_SceneHierarchyPanel.SetSelectedEntity(picked); });
    TrackGizmoEdit();

    // Fermer la fenêtre "Game" avec sa croix arrête le Play, comme sur Godot.
    bool gameStillOpen = m_GamePanel.OnImGuiRender(GetActiveScene(), m_SceneState == SceneState::Play);
    if (m_SceneState == SceneState::Play && !gameStillOpen)
        OnSceneStop();
    m_SceneHierarchyPanel.OnImGuiRender();
    // Historique nul pendant le Play : la scène runtime est jetée au Stop, y annuler
    // une édition n'aurait pas de sens.
    m_InspectorPanel.OnImGuiRender(m_SceneHierarchyPanel.GetSelectedEntity(),
                                   m_SceneState == SceneState::Edit ? &m_CommandHistory : nullptr);
    m_ContentBrowserPanel.OnImGuiRender();
    m_ConsolePanel.OnImGuiRender();

    // Fenêtres du Test Engine (liste des tests, log) : hors dockspace, et seulement
    // en mode interactif.
    m_TestEngine.RenderUI();
}

void EditorLayer::TrackGizmoEdit()
{
    const bool usingGizmo = ImGuizmo::IsUsing();
    Engine::Entity selected = m_SceneHierarchyPanel.GetSelectedEntity();

    if (usingGizmo && !m_GizmoWasUsing && selected)
    {
        m_GizmoEntityID = selected.GetUUID();
        m_TransformBeforeGizmo = selected.GetComponent<Engine::TransformComponent>();
    }
    else if (!usingGizmo && m_GizmoWasUsing && m_SceneState == SceneState::Edit)
    {
        Engine::Entity entity = m_EditorScene->FindEntityByUUID(m_GizmoEntityID);
        if (entity)
        {
            // Le gizmo a déjà écrit la valeur d'arrivée dans le Transform.
            m_CommandHistory.PushAlreadyApplied(std::make_unique<ComponentEditCommand<Engine::TransformComponent>>(
                *this, m_GizmoEntityID, m_TransformBeforeGizmo,
                entity.GetComponent<Engine::TransformComponent>(),
                "manipuler " + entity.GetName()));
        }
    }

    m_GizmoWasUsing = usingGizmo;
}

void EditorLayer::HandleShortcuts()
{
    // Rien pendant le Play : la scène runtime est une copie jetée au Stop, y annuler
    // une action n'aurait aucun effet durable.
    if (m_SceneState != SceneState::Edit)
        return;

    // ImGui::Shortcut plutôt que Engine::Input : le routage évite de déclencher un
    // raccourci pendant qu'on tape dans un champ de texte, et le backend GLFW d'ImGui
    // traduit les touches selon la disposition clavier (donc Ctrl+Z tombe bien sur le
    // Z d'un AZERTY, contrairement aux raccourcis de gizmo qui lisent la touche physique).
    const ImGuiInputFlags route = ImGuiInputFlags_RouteGlobal;

    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Z, route))
        m_CommandHistory.Undo();
    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Y, route) ||
        ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z, route))
        m_CommandHistory.Redo();

    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_C, route))
        CopySelectedEntity();
    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_V, route))
        PasteEntity();
    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_D, route))
        DuplicateSelectedEntity();

    // Suppr n'a pas de modificateur : sans cette garde, il effacerait l'entité
    // sélectionnée pendant qu'on renomme une entité dans l'Inspecteur.
    if (!ImGui::GetIO().WantTextInput && ImGui::Shortcut(ImGuiKey_Delete, route))
        DeleteSelectedEntity();
}

void EditorLayer::CopySelectedEntity()
{
    Engine::Entity selected = m_SceneHierarchyPanel.GetSelectedEntity();
    if (!selected)
        return;

    // Copie hors scène : l'entité d'origine peut être supprimée avant le collage.
    m_Clipboard = std::make_shared<Engine::Scene>();
    m_ClipboardEntity = m_Clipboard->CreateEntity(selected.GetName());
    Engine::Scene::CopyComponents(selected, m_ClipboardEntity);
}

void EditorLayer::PasteEntity()
{
    if (!m_ClipboardEntity)
        return;

    m_CommandHistory.Execute(std::make_unique<CreateEntityFromCommand>(*this, m_ClipboardEntity, "coller"));
}

void EditorLayer::DuplicateSelectedEntity()
{
    Engine::Entity selected = m_SceneHierarchyPanel.GetSelectedEntity();
    if (!selected)
        return;

    m_CommandHistory.Execute(std::make_unique<CreateEntityFromCommand>(*this, selected, "dupliquer"));
}

void EditorLayer::DeleteSelectedEntity()
{
    Engine::Entity selected = m_SceneHierarchyPanel.GetSelectedEntity();
    if (!selected)
        return;

    m_CommandHistory.Execute(std::make_unique<DeleteEntityCommand>(*this, selected));
}

void EditorLayer::RenderViewMenu()
{
    if (!ImGui::BeginMenu("Affichage"))
        return;

    ImGui::MenuItem("Contours des colliders", nullptr, &m_ShowColliderOutlines);

    ImGui::EndMenu();
}

void EditorLayer::RenderEditMenu()
{
    if (!ImGui::BeginMenu("Édition"))
        return;

    const bool editing = m_SceneState == SceneState::Edit;
    const bool hasSelection = editing && (bool)m_SceneHierarchyPanel.GetSelectedEntity();

    // Le nom de la commande apparaît dans l'entrée de menu, façon "Annuler : coller Square".
    const std::string undoName = m_CommandHistory.PeekUndoName();
    const std::string redoName = m_CommandHistory.PeekRedoName();
    const std::string undoLabel = undoName.empty() ? "Annuler" : "Annuler : " + undoName;
    const std::string redoLabel = redoName.empty() ? "Rétablir" : "Rétablir : " + redoName;

    if (ImGui::MenuItem(undoLabel.c_str(), "Ctrl+Z", false, editing && m_CommandHistory.CanUndo()))
        m_CommandHistory.Undo();
    if (ImGui::MenuItem(redoLabel.c_str(), "Ctrl+Y", false, editing && m_CommandHistory.CanRedo()))
        m_CommandHistory.Redo();

    ImGui::Separator();

    if (ImGui::MenuItem("Copier", "Ctrl+C", false, hasSelection))
        CopySelectedEntity();
    if (ImGui::MenuItem("Coller", "Ctrl+V", false, editing && (bool)m_ClipboardEntity))
        PasteEntity();
    if (ImGui::MenuItem("Dupliquer", "Ctrl+D", false, hasSelection))
        DuplicateSelectedEntity();
    if (ImGui::MenuItem("Supprimer", "Suppr", false, hasSelection))
        DeleteSelectedEntity();

    ImGui::EndMenu();
}

void EditorLayer::SetupDefaultDockLayout()
{
    ImGuiID dockspaceId = ImGui::GetID("MyEngineDockSpace");

    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspaceId, ImGui::GetMainViewport()->Size);

    // 3 colonnes : gauche (Viewport / Console), milieu (Hiérarchie / Content Browser), droite (Inspecteur)
    ImGuiID dockRemaining = dockspaceId;
    ImGuiID dockRight = ImGui::DockBuilderSplitNode(dockRemaining, ImGuiDir_Right, 0.2f, nullptr, &dockRemaining);
    ImGuiID dockLeft = ImGui::DockBuilderSplitNode(dockRemaining, ImGuiDir_Left, 0.55f, nullptr, &dockRemaining);
    ImGuiID dockMiddle = dockRemaining;

    ImGuiID dockLeftBottom = ImGui::DockBuilderSplitNode(dockLeft, ImGuiDir_Down, 0.3f, nullptr, &dockLeft);
    ImGuiID dockLeftTop = dockLeft;

    ImGuiID dockMiddleBottom = ImGui::DockBuilderSplitNode(dockMiddle, ImGuiDir_Down, 0.5f, nullptr, &dockMiddle);
    ImGuiID dockMiddleTop = dockMiddle;

    ImGui::DockBuilderDockWindow("Viewport", dockLeftTop);
    // "Game" n'est pas dans le layout par défaut : elle n'existe (et ne se docke jamais)
    // que pendant le Play, en fenêtre flottante détachée — voir GamePanel.
    ImGui::DockBuilderDockWindow("Console", dockLeftBottom);
    ImGui::DockBuilderDockWindow("Hiérarchie de la scène", dockMiddleTop);
    ImGui::DockBuilderDockWindow("Content Browser", dockMiddleBottom);
    ImGui::DockBuilderDockWindow("Inspecteur", dockRight);

    ImGui::DockBuilderFinish(dockspaceId);
}

void EditorLayer::OnScenePlay()
{
    m_SceneState = SceneState::Play;
    m_ScenePaused = false;

    // Scene::Copy() donne à chaque entité un nouveau entt::entity handle (mais garde son
    // UUID) : la sélection courante, qui pointe vers m_EditorScene, doit être retrouvée
    // dans m_RuntimeScene par UUID après la copie — sinon l'Inspecteur continue d'éditer
    // la scène d'édition (jamais utilisée pour le rendu du Play), silencieusement sans effet.
    Engine::Entity previousSelection = m_SceneHierarchyPanel.GetSelectedEntity();
    bool hadSelection = (bool)previousSelection;
    Engine::UUID selectedUUID = hadSelection ? previousSelection.GetUUID() : Engine::UUID(0);

    m_RuntimeScene = m_EditorScene->Copy();
    m_PhysicsSystem.OnRuntimeStart(*m_RuntimeScene);
    m_SceneHierarchyPanel.SetContext(m_RuntimeScene);

    Engine::Entity newSelection;
    if (hadSelection)
        newSelection = m_RuntimeScene->FindEntityByUUID(selectedUUID);
    m_SceneHierarchyPanel.SetSelectedEntity(newSelection);
}

void EditorLayer::OnSceneStop()
{
    m_SceneState = SceneState::Edit;
    m_ScenePaused = false;

    m_PhysicsSystem.OnRuntimeStop();
    m_RuntimeScene = nullptr;
    m_SceneHierarchyPanel.SetContext(m_EditorScene);
    // L'entité sélectionnée appartenait potentiellement à la scène runtime qu'on vient de jeter
    m_SceneHierarchyPanel.SetSelectedEntity({});
}

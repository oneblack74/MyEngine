#include "EditorLayer.h"
#include "Commands/SceneCommands.h"
#include <Core/Log.h>
#include <Assets/AssetManager.h>
#include <Core/Application.h>
#include <Utils/ImageCapture.h>
#include <Renderer/Renderer.h>
#include <Renderer/Renderer2D.h>
#include <Scene/Entity.h>
#include <Scene/Components.h>
#include <Scene/RenderSystem.h>
#include <Scene/SceneManager.h>
#include <filesystem>
#include <imgui.h>
#include <imgui_internal.h> // DockBuilder* : API "interne" ImGui, mais c'est le seul moyen de définir un layout par défaut
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <ImGuizmo.h>
#include <GLFW/glfw3.h>

namespace
{
    // Le suffixe de version invalide les dispositions enregistrées dans imgui.ini quand
    // les panels changent de nom : l'ancien nœud n'est alors plus trouvé, et la
    // disposition par défaut est reconstruite au lieu de laisser les panels renommés
    // flotter hors du dock. À incrémenter à chaque renommage de panel.
    // v2 : passage de l'interface en anglais (Inspecteur -> Inspector, etc.).
    constexpr const char *k_DockSpaceName = "MyEngineDockSpace_v2";
}

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

    // Scène de démo, le temps que SceneHierarchyPanel/ContentBrowserPanel existent.
    // NewScene() fournit déjà la racine unique de la scène : tout le reste se range
    // dessous.
    m_EditorScene = Engine::SceneManager::NewScene();
    Engine::Entity sceneRoot = m_EditorScene->GetRootEntity();

    auto square = m_EditorScene->CreateEntity("Square");
    m_EditorScene->SetParent(square, sceneRoot);
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
    m_EditorScene->SetParent(circle, sceneRoot);
    auto &circleTransform = circle.GetComponent<Engine::TransformComponent>();
    circleTransform.Position = {0.7f, 0.3f, 0.0f};
    circleTransform.Scale = {0.4f, 0.4f, 1.0f};
    circle.AddComponent<Engine::SpriteRendererComponent>(glm::vec4(0.9f, 0.5f, 0.2f, 1.0f));
    auto &circleBody = circle.AddComponent<Engine::RigidBodyComponent>();
    circleBody.Type = Engine::RigidBodyComponent::BodyType::Dynamic;
    circle.AddComponent<Engine::CircleColliderComponent>();

    auto ground = m_EditorScene->CreateEntity("Ground");
    m_EditorScene->SetParent(ground, sceneRoot);
    auto &groundTransform = ground.GetComponent<Engine::TransformComponent>();
    groundTransform.Position = {0.0f, -0.7f, 0.0f};
    groundTransform.Scale = {2.5f, 0.2f, 1.0f};
    ground.AddComponent<Engine::SpriteRendererComponent>(glm::vec4(0.5f, 0.4f, 0.3f, 1.0f));
    ground.AddComponent<Engine::RigidBodyComponent>();
    ground.AddComponent<Engine::BoxColliderComponent>();

    // Démo CameraComponent : sans ça, GamePanel n'a aucune caméra "Primary" à utiliser
    // et affiche juste son avertissement à la place d'un rendu.
    auto mainCamera = m_EditorScene->CreateEntity("Main Camera");
    m_EditorScene->SetParent(mainCamera, sceneRoot);
    mainCamera.AddComponent<Engine::CameraComponent>();

    // Démo Phase 6 : de quoi tester l'audio, mais volontairement muet au lancement du
    // Play — un bip à chaque exécution est vite pénible quand on travaille. Il s'écoute
    // au bouton "Play" de l'Inspecteur, ou en cochant "Play On Start".
    auto sound = m_EditorScene->CreateEntity("Bip");
    m_EditorScene->SetParent(sound, sceneRoot);
    auto &soundAudio = sound.AddComponent<Engine::AudioComponent>();
    soundAudio.Sound = Engine::AssetManager::Import("audio/bip.wav");
    soundAudio.PlayOnStart = false;

    // Démo Phase 5 : logue chaque début/fin de collision dans la Console pour vérifier
    // que les contact events Box2D remontent bien jusqu'à l'ECS (via Entity, pas juste
    // des b2ShapeId bruts).
    m_PhysicsSystem.OnCollisionBegin = [](Engine::Entity a, Engine::Entity b)
    { ENGINE_LOG_INFO(Engine::LogCategories::Collision, "{0} <-> {1}", a.GetName(), b.GetName()); };
    m_PhysicsSystem.OnCollisionEnd = [](Engine::Entity a, Engine::Entity b)
    { ENGINE_LOG_INFO(Engine::LogCategories::Collision, "end: {0} <-> {1}", a.GetName(), b.GetName()); };

    m_SceneHierarchyPanel.SetContext(m_EditorScene, GetSceneDisplayName());
    UpdateWindowTitle();

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
    {
        m_PhysicsSystem.OnUpdate(*m_RuntimeScene, ts);
        Engine::AudioSystem::OnUpdate(*m_RuntimeScene);
    }

    // Rechargement à chaud : un fichier d'asset modifié pendant que l'éditeur tourne
    // est repris sans avoir à redémarrer.
    m_AssetReloadTimer += ts;
    if (m_AssetReloadTimer >= 0.5f)
    {
        m_AssetReloadTimer = 0.0f;
        Engine::AssetManager::ReloadModifiedAssets();
    }

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
    // Le collider de l'entité sélectionnée est toujours visible ; la case du menu
    // Affichage sert à voir ceux de toute la scène d'un coup.
    if (m_ShowColliderOutlines)
        Engine::RenderSystem::RenderColliderOutlines(*GetActiveScene(), *m_Camera);
    else
        Engine::RenderSystem::RenderColliderOutline(m_SceneHierarchyPanel.GetSelectedEntity(), *m_Camera);
    m_Framebuffer->Unbind();

    // Rendu de l'UI ImGui (dockspace + panels) par-dessus
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    // ImGuizmo::BeginFrame() ouvre une fenêtre ImGui "gizmo" de la taille du viewport
    // principal pour y dessiner. Avec ConfigViewportsNoAutoMerge, ImGui refuse de la
    // fondre dans la fenêtre principale et lui fabrique sa propre fenêtre OS — une
    // fenêtre noire plein écran derrière l'éditeur. SetNextWindowViewport est consommé
    // par le prochain Begin(), c'est-à-dire justement celui d'ImGuizmo.
    ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
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
        LOG_ERROR("Test mode: screenshot failed");

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

    ImGuiID dockspaceId = ImGui::GetID(k_DockSpaceName);

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
        RenderFileMenu();
        RenderEditMenu();
        RenderViewMenu();
        if (ImGui::BeginMenu("Window"))
        {
            if (ImGui::MenuItem("Reset Layout"))
                m_ResetDockLayoutRequested = true;
            if (ImGui::MenuItem("Save Layout"))
            {
                ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
                LOG_INFO("Panel layout saved");
            }
            if (ImGui::MenuItem("Load Last Saved Layout"))
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
            if (ImGui::Button(m_ScenePaused ? "Resume" : "Pause"))
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

    // Le dépôt est traité une fois l'arbre entièrement dessiné : rattacher une entité
    // en plein parcours reviendrait à remanier l'arbre pendant qu'on le lit.
    SceneHierarchyPanel::HierarchyDrop drop;
    if (m_SceneHierarchyPanel.TakePendingDrop(drop) && m_SceneState == SceneState::Edit)
        ApplyHierarchyDrop(drop);
    // Historique nul pendant le Play : la scène runtime est jetée au Stop, y annuler
    // une édition n'aurait pas de sens.
    m_InspectorPanel.OnImGuiRender(m_SceneHierarchyPanel.GetSelectedEntity(),
                                   m_SceneState == SceneState::Edit ? &m_CommandHistory : nullptr);
    m_ContentBrowserPanel.OnImGuiRender([this](const std::filesystem::path &scenePath)
                                        {
        // Rien pendant le Play : la scène qui tourne est une copie jetée au Stop.
        if (m_SceneState == SceneState::Edit)
            m_SceneToOpen = scenePath; });
    m_ConsolePanel.OnImGuiRender();

    if (!m_SceneToOpen.empty())
    {
        const std::filesystem::path scenePath = m_SceneToOpen;
        m_SceneToOpen.clear();
        OpenScene(scenePath);
    }

    // La boîte de dialogue est rendue après les panels : c'est une modale, elle doit
    // passer par-dessus tout le reste.
    std::filesystem::path chosenPath;
    if (m_SceneFileDialog.OnImGuiRender(chosenPath))
    {
        if (m_SceneFileDialog.GetMode() == SceneFileDialog::Mode::Open)
            OpenScene(chosenPath);
        else
            SaveSceneTo(chosenPath);
    }

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
                "move " + entity.GetName()));
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

    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_N, route))
        NewScene();
    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_O, route))
        ShowSceneDialog(SceneFileDialog::Mode::Open);
    // Ctrl+Maj+S ne déclenche pas Ctrl+S : ImGui::Shortcut compare l'accord de touches
    // en entier, modificateurs compris.
    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S, route))
        SaveScene();
    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S, route))
        SaveSceneAs();

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

    m_CommandHistory.Execute(std::make_unique<CreateEntityFromCommand>(*this, m_ClipboardEntity, "paste"));
}

void EditorLayer::DuplicateSelectedEntity()
{
    Engine::Entity selected = m_SceneHierarchyPanel.GetSelectedEntity();
    if (!selected)
        return;

    m_CommandHistory.Execute(std::make_unique<CreateEntityFromCommand>(*this, selected, "duplicate"));
}

void EditorLayer::DeleteSelectedEntity()
{
    Engine::Entity selected = m_SceneHierarchyPanel.GetSelectedEntity();
    if (!selected)
        return;

    // Supprimer la racine viderait la scène et lui retirerait ce qui la rend
    // instanciable ailleurs. Godot ne le permet pas non plus.
    if (selected == m_EditorScene->GetRootEntity())
    {
        LOG_WARN("The scene root cannot be deleted");
        return;
    }

    m_CommandHistory.Execute(std::make_unique<DeleteEntityCommand>(*this, selected));
}

void EditorLayer::RenderViewMenu()
{
    if (!ImGui::BeginMenu("View"))
        return;

    ImGui::MenuItem("All collider outlines", nullptr, &m_ShowColliderOutlines);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("The selected entity's outline is always shown");

    ImGui::EndMenu();
}

std::filesystem::path EditorLayer::GetSceneDirectory()
{
    // Les scènes vivent avec les assets du projet : c'est ce que le runtime de la
    // Phase 8 embarquera, et ce que le Content Browser montre déjà.
    return Engine::AssetManager::GetAssetRoot() / "scenes";
}

std::string EditorLayer::GetSceneDisplayName() const
{
    // Le nom du fichier sans extension, comme Unity affiche ses scènes.
    return m_CurrentScenePath.empty() ? "Untitled" : m_CurrentScenePath.stem().string();
}

void EditorLayer::UpdateWindowTitle()
{
    Engine::Application::Get().GetWindow().SetTitle("MyEngine Editor - " + GetSceneDisplayName());
}

void EditorLayer::SetEditorScene(const std::shared_ptr<Engine::Scene> &scene,
                                 const std::filesystem::path &path)
{
    m_EditorScene = scene;
    m_CurrentScenePath = path;
    Engine::SceneManager::SetActiveScene(scene);

    m_SceneHierarchyPanel.SetContext(m_EditorScene, GetSceneDisplayName());
    // La sélection et l'historique portent sur des entités de l'ancienne scène : les
    // garder ferait pointer l'Inspecteur, et surtout les annulations, dans le vide.
    m_SceneHierarchyPanel.SetSelectedEntity({});
    m_CommandHistory.Clear();

    UpdateWindowTitle();
}

void EditorLayer::NewScene()
{
    SetEditorScene(Engine::SceneManager::NewScene(), {});
    LOG_INFO("New scene");
}

void EditorLayer::OpenScene(const std::filesystem::path &path)
{
    auto scene = Engine::SceneManager::LoadScene(path.string());
    if (!scene)
    {
        // SceneManager a déjà logué la cause ; la scène courante reste en place plutôt
        // que de laisser l'éditeur sans scène.
        LOG_ERROR("Open cancelled: {0} could not be loaded", path.string());
        return;
    }

    SetEditorScene(scene, path);
    LOG_INFO("Scene opened: {0}", path.string());
}

void EditorLayer::SaveScene()
{
    if (m_CurrentScenePath.empty())
    {
        SaveSceneAs();
        return;
    }

    SaveSceneTo(m_CurrentScenePath);
}

void EditorLayer::SaveSceneAs()
{
    ShowSceneDialog(SceneFileDialog::Mode::Save);
}

void EditorLayer::SaveSceneTo(const std::filesystem::path &path)
{
    // Le dossier des scènes n'existe pas dans un projet tout neuf : le créer ici évite
    // un échec d'ouverture de flux au premier enregistrement.
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    if (ec)
    {
        LOG_ERROR("Could not create {0}: {1}", path.parent_path().string(), ec.message());
        return;
    }

    Engine::SceneManager::SetActiveScene(m_EditorScene);
    Engine::SceneManager::SaveActiveScene(path.string());

    m_CurrentScenePath = path;
    UpdateWindowTitle();
    LOG_INFO("Scene saved: {0}", path.string());
}

void EditorLayer::ShowSceneDialog(SceneFileDialog::Mode mode)
{
    const std::string suggested =
        m_CurrentScenePath.empty() ? std::string() : m_CurrentScenePath.filename().string();
    m_SceneFileDialog.OpenDialog(mode, GetSceneDirectory(), suggested);
}

void EditorLayer::RenderFileMenu()
{
    if (!ImGui::BeginMenu("File"))
        return;

    // Tout est grisé pendant le Play : la scène éditée n'est pas celle qui tourne, et
    // enregistrer la copie runtime (jetée au Stop) n'aurait aucun sens.
    const bool editing = m_SceneState == SceneState::Edit;

    if (ImGui::MenuItem("New Scene", "Ctrl+N", false, editing))
        NewScene();
    if (ImGui::MenuItem("Open...", "Ctrl+O", false, editing))
        ShowSceneDialog(SceneFileDialog::Mode::Open);

    ImGui::Separator();

    if (ImGui::MenuItem("Save", "Ctrl+S", false, editing))
        SaveScene();
    if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S", false, editing))
        SaveSceneAs();

    ImGui::Separator();

    if (ImGui::MenuItem("Quit"))
        Engine::Application::Get().Close();

    ImGui::EndMenu();
}

void EditorLayer::RenderEditMenu()
{
    if (!ImGui::BeginMenu("Edit"))
        return;

    const bool editing = m_SceneState == SceneState::Edit;
    const bool hasSelection = editing && (bool)m_SceneHierarchyPanel.GetSelectedEntity();

    // Le nom de la commande apparaît dans l'entrée de menu, façon "Annuler : coller Square".
    const std::string undoName = m_CommandHistory.PeekUndoName();
    const std::string redoName = m_CommandHistory.PeekRedoName();
    const std::string undoLabel = undoName.empty() ? "Undo" : "Undo: " + undoName;
    const std::string redoLabel = redoName.empty() ? "Redo" : "Redo: " + redoName;

    if (ImGui::MenuItem(undoLabel.c_str(), "Ctrl+Z", false, editing && m_CommandHistory.CanUndo()))
        m_CommandHistory.Undo();
    if (ImGui::MenuItem(redoLabel.c_str(), "Ctrl+Y", false, editing && m_CommandHistory.CanRedo()))
        m_CommandHistory.Redo();

    ImGui::Separator();

    if (ImGui::MenuItem("Copy", "Ctrl+C", false, hasSelection))
        CopySelectedEntity();
    if (ImGui::MenuItem("Paste", "Ctrl+V", false, editing && (bool)m_ClipboardEntity))
        PasteEntity();
    if (ImGui::MenuItem("Duplicate", "Ctrl+D", false, hasSelection))
        DuplicateSelectedEntity();
    if (ImGui::MenuItem("Delete", "Del", false, hasSelection))
        DeleteSelectedEntity();

    ImGui::EndMenu();
}

void EditorLayer::ApplyHierarchyDrop(const SceneHierarchyPanel::HierarchyDrop &drop)
{
    Engine::Entity dragged = m_EditorScene->FindEntityByUUID(drop.Dragged);
    if (!dragged)
        return;

    Engine::Entity target = m_EditorScene->FindEntityByUUID(drop.Target);

    // Insérer avant une entité, c'est la rejoindre au même niveau : le nouveau parent
    // est alors celui de l'entité de référence, et non elle-même.
    Engine::Entity newParent = drop.InsertBefore ? m_EditorScene->GetParent(target) : target;

    // Déposer sur la scène rattache à son entité racine : une scène n'a qu'une racine,
    // il n'est pas question d'en créer une seconde.
    if (!newParent)
        newParent = m_EditorScene->GetRootEntity();
    if (dragged == newParent)
        return;
    if (!m_EditorScene->CanSetParent(dragged, newParent))
        return;

    m_CommandHistory.Execute(std::make_unique<ReparentEntityCommand>(
        *this, drop.Dragged, newParent ? newParent.GetUUID() : Engine::UUID(0),
        drop.InsertBefore ? drop.Target : Engine::UUID(0)));
}

void EditorLayer::SetupDefaultDockLayout()
{
    ImGuiID dockspaceId = ImGui::GetID(k_DockSpaceName);

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
    ImGui::DockBuilderDockWindow("Scene Hierarchy", dockMiddleTop);
    ImGui::DockBuilderDockWindow("Content Browser", dockMiddleBottom);
    ImGui::DockBuilderDockWindow("Inspector", dockRight);

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
    Engine::AudioSystem::OnRuntimeStart(*m_RuntimeScene);
    m_SceneHierarchyPanel.SetContext(m_RuntimeScene, GetSceneDisplayName());

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
    Engine::AudioSystem::OnRuntimeStop(*m_RuntimeScene);
    m_RuntimeScene = nullptr;
    m_SceneHierarchyPanel.SetContext(m_EditorScene, GetSceneDisplayName());
    // L'entité sélectionnée appartenait potentiellement à la scène runtime qu'on vient de jeter
    m_SceneHierarchyPanel.SetSelectedEntity({});
}

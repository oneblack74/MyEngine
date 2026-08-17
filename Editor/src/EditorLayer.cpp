#include "EditorLayer.h"
#include <Core/Log.h>
#include <Core/Application.h>
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

EditorLayer::EditorLayer() : Layer("EditorLayer") {}

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

    ImGui::StyleColorsDark();

    GLFWwindow *window = Engine::Application::Get().GetWindow().GetNativeWindow();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void EditorLayer::OnDetach()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    Engine::Renderer2D::Shutdown();
}

void EditorLayer::OnUpdate(Engine::Timestep ts)
{
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
    m_Framebuffer->Unbind();

    // Rendu de l'UI ImGui (dockspace + panels) par-dessus
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame(); // requis une fois par frame pour qu'ImGuizmo suive le drag de la souris

    RenderImGui();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void EditorLayer::RenderImGui()
{
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
    m_SceneHierarchyPanel.OnImGuiRender();
    m_InspectorPanel.OnImGuiRender(m_SceneHierarchyPanel.GetSelectedEntity());
    m_ContentBrowserPanel.OnImGuiRender();
    m_ConsolePanel.OnImGuiRender();
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

    m_RuntimeScene = m_EditorScene->Copy();
    m_PhysicsSystem.OnRuntimeStart(*m_RuntimeScene);
    m_SceneHierarchyPanel.SetContext(m_RuntimeScene);
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

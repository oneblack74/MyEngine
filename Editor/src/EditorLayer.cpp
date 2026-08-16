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
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
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
    m_ActiveScene = Engine::SceneManager::NewScene();
    auto square = m_ActiveScene->CreateEntity("Square");
    auto &transform = square.GetComponent<Engine::TransformComponent>();
    transform.Scale = {0.5f, 0.5f, 1.0f};
    square.AddComponent<Engine::SpriteRendererComponent>(glm::vec4(0.2f, 0.6f, 0.9f, 1.0f));

    m_SceneHierarchyPanel.SetContext(m_ActiveScene);

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

void EditorLayer::OnUpdate()
{
    // Redimensionne le Framebuffer si le panel Viewport a changé de taille
    const glm::vec2 &viewportSize = m_ViewportPanel.GetSize();
    const auto &spec = m_Framebuffer->GetSpecification();
    if (viewportSize.x > 0.0f && viewportSize.y > 0.0f &&
        (spec.Width != (uint32_t)viewportSize.x || spec.Height != (uint32_t)viewportSize.y))
    {
        m_Framebuffer->Resize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);

        // La projection de la caméra doit suivre le nouveau ratio largeur/hauteur du
        // panel, sinon l'image rendue est étirée pour remplir un cadre qui n'a pas
        // le même ratio que ce pour quoi elle a été projetée.
        float aspectRatio = viewportSize.x / viewportSize.y;
        constexpr float zoom = 0.9f;
        m_Camera->SetProjection(-aspectRatio * zoom, aspectRatio * zoom, -zoom, zoom);
    }

    // Rendu de la scène hors écran, dans le Framebuffer
    m_Framebuffer->Bind();
    Engine::Renderer::SetClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    Engine::Renderer::Clear();
    Engine::RenderSystem::Render(*m_ActiveScene, *m_Camera);
    m_Framebuffer->Unbind();

    // Rendu de l'UI ImGui (dockspace + panels) par-dessus
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

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
    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f));

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("Fichier"))
        {
            ImGui::MenuItem("Quitter (bientôt)", nullptr, false, false);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGui::End();

    m_ViewportPanel.OnImGuiRender(m_Framebuffer);
    m_SceneHierarchyPanel.OnImGuiRender();
    m_InspectorPanel.OnImGuiRender(m_SceneHierarchyPanel.GetSelectedEntity());
    m_ContentBrowserPanel.OnImGuiRender();
    m_ConsolePanel.OnImGuiRender();
}

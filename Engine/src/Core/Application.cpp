#include "Core/Application.h"
#include "Assets/AssetManager.h"
#include "Audio/AudioEngine.h"
#include "Core/Log.h"
#include <GLFW/glfw3.h>
#include <cassert>

namespace Engine
{
    Application *Application::s_Instance = nullptr;

    Application::Application(const WindowProps &props, const std::filesystem::path &assetRoot)
        : m_Window(props)
    {
        assert(!s_Instance && "An Application already exists!");
        s_Instance = this;

        m_Window.SetEventCallback([this](Event &e)
                                  { OnEvent(e); });

        AudioEngine::Init();
        AssetManager::Init(assetRoot);
    }

    Application::~Application()
    {
        AssetManager::Shutdown();
        AudioEngine::Shutdown();
    }

    void Application::OnEvent(Event &event)
    {
        // Les événements sont catégorisés par leur nature : le clavier et la souris
        // sous Input, le reste sous Window. Sans ça ils tombaient tous dans "General"
        // et noyaient la console sans pouvoir être filtrés.
        const bool isInput = event.GetType() != EventType::WindowClose &&
                             event.GetType() != EventType::WindowResize;
        ENGINE_LOG_TRACE(isInput ? LogCategories::Input : LogCategories::Window, event.ToString());

        if (event.GetType() == EventType::WindowClose)
            m_Running = false;

        // Propager aux layers en partant du haut, jusqu'à ce qu'un layer marque l'event comme géré
        for (auto it = m_LayerStack.end(); it != m_LayerStack.begin();)
        {
            --it;
            (*it)->OnEvent(event);
            if (event.Handled)
                break;
        }
    }

    void Application::PushLayer(Layer *layer)
    {
        m_LayerStack.PushLayer(layer);
    }

    void Application::PushOverlay(Layer *overlay)
    {
        m_LayerStack.PushOverlay(overlay);
    }

    void Application::Run()
    {
        float lastFrameTime = (float)glfwGetTime();

        while (m_Running)
        {
            float time = (float)glfwGetTime();
            Timestep timestep = time - lastFrameTime;
            lastFrameTime = time;

            for (Engine::Layer *layer : m_LayerStack)
                layer->OnUpdate(timestep);

            m_Window.OnUpdate();
        }
    }
}

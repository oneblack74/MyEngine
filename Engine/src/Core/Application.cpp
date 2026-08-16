#include "Core/Application.h"
#include "Core/Log.h"

namespace Engine
{
    Application::Application()
        : m_Window(WindowProps())
    {
        m_Window.SetEventCallback([this](Event &e)
                                  { OnEvent(e); });
    }

    Application::~Application() {}

    void Application::OnEvent(Event &event)
    {
        LOG_TRACE(event.ToString());

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
        while (m_Running)
        {
            for (Engine::Layer *layer : m_LayerStack)
                layer->OnUpdate();

            m_Window.OnUpdate();
        }
    }
}

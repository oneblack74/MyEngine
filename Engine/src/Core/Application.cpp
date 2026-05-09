#include "Core/Application.h"
#include <iostream>

namespace MyEngine
{
    Application::Application()
        : m_Window(Engine::WindowProps())
    {
        m_Window.SetEventCallback([this](Engine::Event &e)
                                  { OnEvent(e); });
    }

    Application::~Application() {}

    void Application::OnEvent(Engine::Event &event)
    {
        std::cout << event.ToString() << std::endl;

        if (event.GetType() == Engine::EventType::WindowClose)
            m_Running = false;

        // Propager aux layers en partant du haut
        for (auto it = m_LayerStack.end(); it != m_LayerStack.begin();)
        {
            --it;
            (*it)->OnEvent(event);
        }
    }

    void Application::PushLayer(Engine::Layer *layer)
    {
        m_LayerStack.PushLayer(layer);
    }

    void Application::PushOverlay(Engine::Layer *overlay)
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

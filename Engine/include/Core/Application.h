#pragma once
#include "Core/Window.h"
#include "Core/LayerStack.h"

namespace MyEngine
{
    class Application
    {
    public:
        Application();
        ~Application();

        void Run();
        void OnEvent(Engine::Event &event);

        void PushLayer(Engine::Layer *layer);
        void PushOverlay(Engine::Layer *overlay);

    private:
        Engine::Window m_Window;
        Engine::LayerStack m_LayerStack;
        bool m_Running = true;
    };
}

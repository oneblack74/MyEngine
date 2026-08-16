#pragma once
#include "Core/Window.h"
#include "Core/LayerStack.h"

namespace Engine
{
    class Application
    {
    public:
        Application();
        ~Application();

        void Run();
        void OnEvent(Event &event);

        void PushLayer(Layer *layer);
        void PushOverlay(Layer *overlay);

    private:
        Window m_Window;
        LayerStack m_LayerStack;
        bool m_Running = true;
    };
}

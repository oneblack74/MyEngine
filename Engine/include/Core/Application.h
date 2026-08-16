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

        Window &GetWindow() { return m_Window; }
        static Application &Get() { return *s_Instance; }

    private:
        Window m_Window;
        LayerStack m_LayerStack;
        bool m_Running = true;

        static Application *s_Instance;
    };
}

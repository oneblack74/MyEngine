#pragma once
#include "Core/Window.h"
#include "Core/LayerStack.h"

namespace Engine
{
    class Application
    {
    public:
        Application(const WindowProps &props = WindowProps());
        ~Application();

        void Run();
        // Demande l'arrêt de la boucle principale à la fin de la frame courante.
        void Close() { m_Running = false; }
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

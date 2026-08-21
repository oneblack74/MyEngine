#pragma once
#include "Core/Window.h"
#include "Core/LayerStack.h"
#include <filesystem>

namespace Engine
{
    class Application
    {
    public:
        // assetRoot est la racine des assets du projet. Un build de développement la fait
        // pointer sur les sources, pour que le registre soit versionné avec elles et
        // qu'éditer une image se voie sans passer par une recopie.
        Application(const WindowProps &props = WindowProps(),
                    const std::filesystem::path &assetRoot = "assets");
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

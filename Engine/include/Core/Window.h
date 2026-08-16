#pragma once
#include <string>
#include <functional>
#include "Events/Event.h"
#include "Events/WindowEvent.h"
#include "Events/KeyEvent.h"

struct GLFWwindow;

namespace Engine
{
    struct WindowProps
    {
        std::string Title;
        unsigned int Width;
        unsigned int Height;

        WindowProps(const std::string &title = "MyEngine",
                    unsigned int width = 1280,
                    unsigned int height = 720)
            : Title(title), Width(width), Height(height) {}
    };

    class Window
    {
    public:
        Window(const WindowProps &props);
        ~Window();

        void OnUpdate();

        unsigned int GetWidth() const { return m_Data.Width; }
        unsigned int GetHeight() const { return m_Data.Height; }
        bool ShouldClose() const;

        GLFWwindow *GetNativeWindow() const { return m_Window; }

        void SetEventCallback(const std::function<void(Event &)> &callback)
        {
            m_Data.EventCallback = callback;
        }

    private:
        void Init(const WindowProps &props);
        void Shutdown();

        struct WindowData
        {
            std::string Title;
            unsigned int Width, Height;
            std::function<void(Event &)> EventCallback;
        };

        WindowData m_Data;
        GLFWwindow *m_Window; // ← GLFWwindow* pas void*
    };
}

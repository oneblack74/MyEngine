#pragma once
#include <string>

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

private:
    void Init(const WindowProps &props);
    void Shutdown();

    struct WindowData
    {
        std::string Title;
        unsigned int Width, Height;
    };

    WindowData m_Data;
    void *m_Window; // GLFWwindow*
};

#include "Core/Window.h"
#include <GLFW/glfw3.h>
#include <iostream>

Window::Window(const WindowProps &props)
{
    Init(props);
}

Window::~Window()
{
    Shutdown();
}

void Window::Init(const WindowProps &props)
{
    m_Data.Title = props.Title;
    m_Data.Width = props.Width;
    m_Data.Height = props.Height;

    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW!" << std::endl;
        return;
    }

    m_Window = glfwCreateWindow(m_Data.Width, m_Data.Height,
                                m_Data.Title.c_str(), nullptr, nullptr);

    if (!m_Window)
    {
        std::cerr << "Failed to create window!" << std::endl;
        return;
    }

    glfwMakeContextCurrent((GLFWwindow *)m_Window);
}

void Window::Shutdown()
{
    glfwDestroyWindow((GLFWwindow *)m_Window);
    glfwTerminate();
}

void Window::OnUpdate()
{
    glfwPollEvents();
    glfwSwapBuffers((GLFWwindow *)m_Window);
}

bool Window::ShouldClose() const
{
    return glfwWindowShouldClose((GLFWwindow *)m_Window);
}

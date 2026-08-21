#include "Core/Window.h"
#include "Core/Log.h"
#include "glad/gl.h"
#include <GLFW/glfw3.h>
#include <stdexcept>

namespace Engine
{
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
            LOG_ERROR("Failed to initialize GLFW!");
            throw std::runtime_error("Failed to initialize GLFW");
        }

        glfwDefaultWindowHints();
        glfwWindowHint(GLFW_VISIBLE, props.Visible ? GLFW_TRUE : GLFW_FALSE);

        m_Window = glfwCreateWindow(m_Data.Width, m_Data.Height,
                                    m_Data.Title.c_str(), nullptr, nullptr);

        if (!m_Window)
        {
            LOG_ERROR("Failed to create window!");
            throw std::runtime_error("Failed to create window");
        }

        glfwMakeContextCurrent(m_Window);
        if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress))
        {
            LOG_ERROR("Failed to initialize Glad!");
            throw std::runtime_error("Failed to initialize Glad");
        }

        glfwSetWindowUserPointer(m_Window, &m_Data);

        glfwSetWindowCloseCallback(m_Window, [](GLFWwindow *window)
                                   {
            WindowData &data = *(WindowData *)glfwGetWindowUserPointer(window);
            WindowCloseEvent event;
            data.EventCallback(event); });

        glfwSetWindowSizeCallback(m_Window, [](GLFWwindow *window, int width, int height)
                                  {
            WindowData &data = *(WindowData *)glfwGetWindowUserPointer(window);
            WindowResizeEvent event(width, height);
            data.EventCallback(event); });

        glfwSetKeyCallback(m_Window, [](GLFWwindow *window, int key, int scancode, int action, int mods)
                           {
            WindowData &data = *(WindowData *)glfwGetWindowUserPointer(window);
            if (action == GLFW_PRESS)
            {
                KeyPressedEvent event(key);
                data.EventCallback(event);
            }
            else if (action == GLFW_RELEASE)
            {
                KeyReleasedEvent event(key);
                if (data.EventCallback)
                {
                    data.EventCallback(event);
                }
            } });
    }

    void Window::Shutdown()
    {
        glfwDestroyWindow(m_Window);
        glfwTerminate();
    }

    void Window::OnUpdate()
    {
        glfwPollEvents();
        glfwSwapBuffers(m_Window);
    }

    bool Window::ShouldClose() const
    {
        return glfwWindowShouldClose(m_Window);
    }
}

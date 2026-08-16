#include "Core/Input.h"
#include "Core/Application.h"
#include <GLFW/glfw3.h>

namespace Engine
{
    bool Input::IsKeyPressed(KeyCode key)
    {
        GLFWwindow *window = Application::Get().GetWindow().GetNativeWindow();
        int state = glfwGetKey(window, key);
        return state == GLFW_PRESS || state == GLFW_REPEAT;
    }

    bool Input::IsMouseButtonPressed(MouseCode button)
    {
        GLFWwindow *window = Application::Get().GetWindow().GetNativeWindow();
        int state = glfwGetMouseButton(window, button);
        return state == GLFW_PRESS;
    }

    glm::vec2 Input::GetMousePosition()
    {
        GLFWwindow *window = Application::Get().GetWindow().GetNativeWindow();
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        return {(float)x, (float)y};
    }

    float Input::GetMouseX()
    {
        return GetMousePosition().x;
    }

    float Input::GetMouseY()
    {
        return GetMousePosition().y;
    }
}

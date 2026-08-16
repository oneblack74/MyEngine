#pragma once
#include "Core/KeyCodes.h"
#include "Core/MouseCodes.h"
#include <glm/glm.hpp>

namespace Engine
{
    // Polling clavier/souris : interroge l'état courant à tout moment (contrairement
    // aux events, qui ne signalent qu'un changement d'état).
    class Input
    {
    public:
        static bool IsKeyPressed(KeyCode key);
        static bool IsMouseButtonPressed(MouseCode button);
        static glm::vec2 GetMousePosition();
        static float GetMouseX();
        static float GetMouseY();
    };
}

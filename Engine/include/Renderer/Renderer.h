#pragma once
#include "RenderCommand.h"

namespace Engine
{
    class Renderer
    {
    public:
        static void BeginScene() {} // plus tard : camera, lights
        static void EndScene() {}

        static void SetClearColor(float r, float g, float b, float a)
        {
            RenderCommand::SetClearColor(r, g, b, a);
        }

        static void Clear()
        {
            RenderCommand::Clear();
        }
    };
}

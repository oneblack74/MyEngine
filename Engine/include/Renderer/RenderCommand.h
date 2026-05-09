#pragma once
#include <glad/gl.h>

namespace Engine
{
    class RenderCommand
    {
    public:
        static void SetClearColor(float r, float g, float b, float a)
        {
            glClearColor(r, g, b, a);
        }

        static void Clear()
        {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }
    };
}

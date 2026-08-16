#pragma once
#include "Renderer/VertexArray.h"
#include <glad/gl.h>
#include <memory>

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

        static void DrawIndexed(const std::shared_ptr<VertexArray> &vertexArray)
        {
            glDrawElements(GL_TRIANGLES, vertexArray->GetIndexBuffer()->GetCount(), GL_UNSIGNED_INT, nullptr);
        }
    };
}

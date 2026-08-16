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

        // indexCount = 0 signifie "dessiner tout l'index buffer" (cas standard, non batché)
        static void DrawIndexed(const std::shared_ptr<VertexArray> &vertexArray, uint32_t indexCount = 0)
        {
            uint32_t count = indexCount ? indexCount : vertexArray->GetIndexBuffer()->GetCount();
            glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
        }
    };
}

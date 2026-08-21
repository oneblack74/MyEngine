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
            // Le VAO doit être lié ici : depuis qu'il en existe plusieurs (quads et
            // lignes), on ne peut plus compter sur celui laissé lié par l'initialisation.
            vertexArray->Bind();
            uint32_t count = indexCount ? indexCount : vertexArray->GetIndexBuffer()->GetCount();
            glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
        }

        // Lignes : pas d'index buffer, les sommets vont deux par deux.
        static void DrawLines(const std::shared_ptr<VertexArray> &vertexArray, uint32_t vertexCount)
        {
            vertexArray->Bind();
            glDrawArrays(GL_LINES, 0, vertexCount);
        }

        static void SetLineWidth(float width)
        {
            glLineWidth(width);
        }
    };
}

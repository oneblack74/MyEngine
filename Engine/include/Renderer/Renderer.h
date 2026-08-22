#pragma once
#include "RenderCommand.h"
#include "VertexArray.h"
#include <memory>

namespace Engine
{
    class Renderer
    {
    public:
        static void BeginScene() {} // plus tard : camera, lights
        static void EndScene() {}

        static void Submit(const std::shared_ptr<VertexArray> &vertexArray)
        {
            vertexArray->Bind();
            RenderCommand::DrawIndexed(vertexArray);
        }

        static void SetClearColor(float r, float g, float b, float a)
        {
            RenderCommand::SetClearColor(r, g, b, a);
        }

        static void Clear()
        {
            RenderCommand::Clear();
        }

        static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
        {
            RenderCommand::SetViewport(x, y, width, height);
        }
    };
}

#pragma once
#include <memory>
#include "Renderer/Buffer.h"

namespace Engine
{
    class VertexArray
    {
    public:
        VertexArray();
        ~VertexArray();

        void Bind() const;
        void Unbind() const;

        void AddVertexBuffer(const std::shared_ptr<VertexBuffer> &vb);
        void SetIndexBuffer(const std::shared_ptr<IndexBuffer> &ib);

        const std::shared_ptr<IndexBuffer> &GetIndexBuffer() const { return m_IndexBuffer; }

    private:
        uint32_t m_RendererID;
        std::shared_ptr<VertexBuffer> m_VertexBuffer;
        std::shared_ptr<IndexBuffer> m_IndexBuffer;
    };
}

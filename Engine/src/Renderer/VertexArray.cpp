#include "Renderer/VertexArray.h"
#include <glad/gl.h>

namespace Engine
{
    VertexArray::VertexArray()
    {
        glGenVertexArrays(1, &m_RendererID);
    }

    VertexArray::~VertexArray()
    {
        glDeleteVertexArrays(1, &m_RendererID);
    }

    void VertexArray::Bind() const
    {
        glBindVertexArray(m_RendererID);
    }

    void VertexArray::Unbind() const
    {
        glBindVertexArray(0);
    }

    void VertexArray::AddVertexBuffer(const std::shared_ptr<VertexBuffer> &vb)
    {
        glBindVertexArray(m_RendererID);
        vb->Bind();

        const auto &layout = vb->GetLayout();
        uint32_t index = 0;
        for (const auto &element : layout.GetElements())
        {
            glEnableVertexAttribArray(index);
            glVertexAttribPointer(
                index,
                element.GetComponentCount(),
                GL_FLOAT,
                GL_FALSE,
                layout.GetStride(),
                (void *)(intptr_t)element.Offset);
            index++;
        }

        m_VertexBuffer = vb;
    }

    void VertexArray::SetIndexBuffer(const std::shared_ptr<IndexBuffer> &ib)
    {
        glBindVertexArray(m_RendererID);
        ib->Bind();
        m_IndexBuffer = ib;
    }
}

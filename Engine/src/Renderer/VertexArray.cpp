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

        // Position XYZ — layout location 0
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);

        m_VertexBuffer = vb;
    }

    void VertexArray::SetIndexBuffer(const std::shared_ptr<IndexBuffer> &ib)
    {
        glBindVertexArray(m_RendererID);
        ib->Bind();
        m_IndexBuffer = ib;
    }
}

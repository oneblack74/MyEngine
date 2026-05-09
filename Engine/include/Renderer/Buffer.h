#pragma once
#include <cstdint>
#include <vector>
#include <string>

namespace Engine
{
    enum class ShaderDataType
    {
        Float,
        Float2,
        Float3,
        Float4
    };

    static uint32_t ShaderDataTypeSize(ShaderDataType type)
    {
        switch (type)
        {
        case ShaderDataType::Float:
            return 4;
        case ShaderDataType::Float2:
            return 8;
        case ShaderDataType::Float3:
            return 12;
        case ShaderDataType::Float4:
            return 16;
        }
        return 0;
    }

    struct BufferElement
    {
        std::string Name;
        ShaderDataType Type;
        uint32_t Size;
        uint32_t Offset;

        BufferElement(ShaderDataType type, const std::string &name)
            : Name(name), Type(type), Size(ShaderDataTypeSize(type)), Offset(0) {}

        uint32_t GetComponentCount() const
        {
            switch (Type)
            {
            case ShaderDataType::Float:
                return 1;
            case ShaderDataType::Float2:
                return 2;
            case ShaderDataType::Float3:
                return 3;
            case ShaderDataType::Float4:
                return 4;
            }
            return 0;
        }
    };

    class BufferLayout
    {
    public:
        BufferLayout() {}
        BufferLayout(std::initializer_list<BufferElement> elements)
            : m_Elements(elements)
        {
            CalculateOffsetsAndStride();
        }

        const std::vector<BufferElement> &GetElements() const { return m_Elements; }
        uint32_t GetStride() const { return m_Stride; }

    private:
        void CalculateOffsetsAndStride()
        {
            uint32_t offset = 0;
            m_Stride = 0;
            for (auto &e : m_Elements)
            {
                e.Offset = offset;
                offset += e.Size;
                m_Stride += e.Size;
            }
        }

        std::vector<BufferElement> m_Elements;
        uint32_t m_Stride = 0;
    };

    class VertexBuffer
    {
    public:
        VertexBuffer(float *vertices, uint32_t size);
        ~VertexBuffer();

        void Bind() const;
        void Unbind() const;

        void SetLayout(const BufferLayout &layout) { m_Layout = layout; }
        const BufferLayout &GetLayout() const { return m_Layout; }

    private:
        uint32_t m_RendererID;
        BufferLayout m_Layout;
    };

    class IndexBuffer
    {
    public:
        IndexBuffer(uint32_t *indices, uint32_t count);
        ~IndexBuffer();

        void Bind() const;
        void Unbind() const;

        uint32_t GetCount() const { return m_Count; }

    private:
        uint32_t m_RendererID;
        uint32_t m_Count;
    };
}

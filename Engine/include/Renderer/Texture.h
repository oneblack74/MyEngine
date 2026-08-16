#pragma once
#include <string>
#include <cstdint>

namespace Engine
{

    class Texture2D
    {
    public:
        Texture2D(const std::string &path);
        Texture2D(uint32_t width, uint32_t height); // texture vide, remplie via SetData
        ~Texture2D();

        void Bind(uint32_t slot = 0) const;
        void SetData(const void *data, uint32_t size);

        uint32_t GetWidth() const { return m_Width; }
        uint32_t GetHeight() const { return m_Height; }
        uint32_t GetRendererID() const { return m_RendererID; }

    private:
        uint32_t m_RendererID;
        uint32_t m_Width, m_Height;
    };

} // namespace Engine

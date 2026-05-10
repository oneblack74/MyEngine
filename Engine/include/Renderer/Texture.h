#pragma once
#include <string>
#include <cstdint>

namespace Engine
{

    class Texture2D
    {
    public:
        Texture2D(const std::string &path);
        ~Texture2D();

        void Bind(uint32_t slot = 0) const;

        uint32_t GetWidth() const { return m_Width; }
        uint32_t GetHeight() const { return m_Height; }

    private:
        uint32_t m_RendererID;
        uint32_t m_Width, m_Height;
    };

} // namespace Engine

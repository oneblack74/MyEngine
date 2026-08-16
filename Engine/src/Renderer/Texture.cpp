#include "Renderer/Texture.h"
#include "stb_image.h"
#include <glad/gl.h>
#include <stdexcept>

namespace Engine
{

    Texture2D::Texture2D(const std::string &path)
    {
        int width, height, channels;
        stbi_set_flip_vertically_on_load(true);
        unsigned char *data = stbi_load(path.c_str(), &width, &height, &channels, 4);

        if (!data)
            throw std::runtime_error("Failed to load texture: " + path);

        m_Width = width;
        m_Height = height;

        // stbi_load est forcé à 4 canaux (desired_channels=4 ci-dessus),
        // donc le buffer est toujours en RGBA quel que soit le fichier source.
        glGenTextures(1, &m_RendererID);
        glBindTexture(GL_TEXTURE_2D, m_RendererID);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

        stbi_image_free(data);
    }

    Texture2D::~Texture2D()
    {
        glDeleteTextures(1, &m_RendererID);
    }

    void Texture2D::Bind(uint32_t slot) const
    {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, m_RendererID);
    }

} // namespace Engine

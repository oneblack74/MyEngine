#include "Utils/ImageCapture.h"
#include "Core/Log.h"
#include "glad/gl.h"
#include <stb_image_write.h>
#include <filesystem>
#include <vector>

namespace Engine::ImageCapture
{
    bool CaptureBackBuffer(const std::string &path, uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0)
        {
            LOG_ERROR("ImageCapture : taille invalide ({0}x{1})", width, height);
            return false;
        }

        std::vector<uint8_t> pixels((size_t)width * height * 4);

        glReadBuffer(GL_BACK);
        // Par défaut GL aligne le début de chaque ligne sur 4 octets ; en RGBA une ligne
        // fait déjà un multiple de 4, mais on le fixe pour ne pas dépendre de la largeur.
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, (GLsizei)width, (GLsizei)height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

        return WriteRGBA(path, width, height, pixels.data(), true);
    }

    bool WriteRGBA(const std::string &path, uint32_t width, uint32_t height,
                   const uint8_t *pixels, bool flipVertically)
    {
        const std::filesystem::path outPath(path);
        if (outPath.has_parent_path())
        {
            std::error_code ec;
            std::filesystem::create_directories(outPath.parent_path(), ec);
        }

        const int stride = (int)width * 4;
        stbi_flip_vertically_on_write(flipVertically ? 1 : 0);
        const int ok = stbi_write_png(path.c_str(), (int)width, (int)height, 4, pixels, stride);
        stbi_flip_vertically_on_write(0);

        if (!ok)
        {
            LOG_ERROR("ImageCapture : échec de l'écriture de '{0}'", path);
            return false;
        }

        LOG_INFO("Capture écrite : {0} ({1}x{2})", path, width, height);
        return true;
    }
}

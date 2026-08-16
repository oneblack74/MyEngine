#pragma once
#include "Renderer/Texture.h"
#include <glm/glm.hpp>
#include <memory>

namespace Engine
{
    // Représente une sous-région (sprite) d'une texture plus grande (sprite sheet / atlas),
    // sous forme de 4 coordonnées UV dans la texture source.
    class SubTexture2D
    {
    public:
        SubTexture2D(const std::shared_ptr<Texture2D> &texture, const glm::vec2 &min, const glm::vec2 &max);

        const std::shared_ptr<Texture2D> &GetTexture() const { return m_Texture; }
        const glm::vec2 *GetTexCoords() const { return m_TexCoords; }

        // coords = position de la cellule dans la grille (colonne, ligne)
        // cellSize = taille d'une cellule en pixels
        // spriteSize = nombre de cellules occupées par le sprite (1x1 par défaut)
        static std::shared_ptr<SubTexture2D> CreateFromCoords(const std::shared_ptr<Texture2D> &texture,
                                                                const glm::vec2 &coords,
                                                                const glm::vec2 &cellSize,
                                                                const glm::vec2 &spriteSize = {1.0f, 1.0f});

    private:
        std::shared_ptr<Texture2D> m_Texture;
        glm::vec2 m_TexCoords[4];
    };
}

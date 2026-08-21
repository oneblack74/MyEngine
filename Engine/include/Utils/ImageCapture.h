#pragma once
#include <cstdint>
#include <string>

namespace Engine::ImageCapture
{
    // Lit le back buffer de la fenêtre GL courante et l'écrit en PNG.
    // À appeler après le rendu de la frame et AVANT le swap : après le swap, ce qu'on
    // vient de dessiner est passé dans le front buffer et GL_BACK contient autre chose.
    bool CaptureBackBuffer(const std::string &path, uint32_t width, uint32_t height);

    // Écrit un buffer RGBA déjà en main. flipVertically pour les pixels venant d'OpenGL,
    // dont l'origine est en bas à gauche alors que le PNG attend la première ligne en haut.
    bool WriteRGBA(const std::string &path, uint32_t width, uint32_t height,
                   const uint8_t *pixels, bool flipVertically);
}

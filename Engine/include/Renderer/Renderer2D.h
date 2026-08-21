#pragma once
#include "Renderer/OrthographicCamera.h"
#include "Renderer/Texture.h"
#include "Renderer/SubTexture2D.h"
#include <glm/glm.hpp>
#include <memory>

namespace Engine
{
    class Renderer2D
    {
    public:
        static void Init();
        static void Shutdown();

        static void BeginScene(const OrthographicCamera &camera);
        static void EndScene();
        static void Flush();

        // Quad coloré (texture blanche 1x1)
        static void DrawQuad(const glm::vec2 &position, const glm::vec2 &size, const glm::vec4 &color);
        static void DrawQuad(const glm::vec3 &position, const glm::vec2 &size, const glm::vec4 &color);

        // Quad texturé
        static void DrawQuad(const glm::vec2 &position, const glm::vec2 &size,
                              const std::shared_ptr<Texture2D> &texture,
                              float tilingFactor = 1.0f,
                              const glm::vec4 &tintColor = glm::vec4(1.0f));
        static void DrawQuad(const glm::vec3 &position, const glm::vec2 &size,
                              const std::shared_ptr<Texture2D> &texture,
                              float tilingFactor = 1.0f,
                              const glm::vec4 &tintColor = glm::vec4(1.0f));

        // Quad affichant une sous-région d'une texture (sprite sheet)
        static void DrawQuad(const glm::vec2 &position, const glm::vec2 &size,
                              const std::shared_ptr<SubTexture2D> &subTexture,
                              float tilingFactor = 1.0f,
                              const glm::vec4 &tintColor = glm::vec4(1.0f));
        static void DrawQuad(const glm::vec3 &position, const glm::vec2 &size,
                              const std::shared_ptr<SubTexture2D> &subTexture,
                              float tilingFactor = 1.0f,
                              const glm::vec4 &tintColor = glm::vec4(1.0f));

        // Quad à partir d'une transform complète (position/rotation/scale) — utilisé par l'ECS
        static void DrawQuad(const glm::mat4 &transform, const glm::vec4 &color);
        static void DrawQuad(const glm::mat4 &transform, const std::shared_ptr<Texture2D> &texture,
                              float tilingFactor = 1.0f,
                              const glm::vec4 &tintColor = glm::vec4(1.0f));

        // --- Tracés en fil de fer ---
        // Batchés à part des quads : ils passent par leur propre shader et un
        // glDrawArrays(GL_LINES), là où les quads sont indexés en triangles.

        static void DrawLine(const glm::vec3 &p0, const glm::vec3 &p1, const glm::vec4 &color);

        // Contour du quad unité [-0.5, 0.5] transformé — suit donc la rotation et
        // l'échelle de la transform, contrairement à un rectangle aligné aux axes.
        static void DrawRect(const glm::mat4 &transform, const glm::vec4 &color);
        static void DrawRect(const glm::vec3 &center, const glm::vec2 &size, const glm::vec4 &color);

        // Cercle approximé par des segments : assez pour un contour de collider.
        static void DrawCircle(const glm::vec3 &center, float radius, const glm::vec4 &color,
                                int segments = 32);

        static void SetLineWidth(float width);

    private:
        static void FlushQuads();
        static void FlushLines();
        static void StartBatch();
        static void NextBatch();

        static void DrawQuadTexture(const glm::mat4 &transform,
                                     const std::shared_ptr<Texture2D> &texture,
                                     const glm::vec2 *texCoords,
                                     float tilingFactor, const glm::vec4 &tintColor);
    };
}

#pragma once
#include "Renderer/OrthographicCamera.h"
#include "Renderer/Texture.h"
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

    private:
        static void StartBatch();
        static void NextBatch();
    };
}

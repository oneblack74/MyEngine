#pragma once
#include "Renderer/Texture.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <string>

namespace Engine
{
    struct TagComponent
    {
        std::string Tag;

        TagComponent() = default;
        TagComponent(const TagComponent &) = default;
        TagComponent(const std::string &tag) : Tag(tag) {}
    };

    struct TransformComponent
    {
        glm::vec3 Position = {0.0f, 0.0f, 0.0f};
        float Rotation = 0.0f; // en degrés, autour de Z (moteur 2D)
        glm::vec3 Scale = {1.0f, 1.0f, 1.0f};

        TransformComponent() = default;
        TransformComponent(const TransformComponent &) = default;
        TransformComponent(const glm::vec3 &position) : Position(position) {}

        glm::mat4 GetTransform() const
        {
            return glm::translate(glm::mat4(1.0f), Position) *
                   glm::rotate(glm::mat4(1.0f), glm::radians(Rotation), glm::vec3(0.0f, 0.0f, 1.0f)) *
                   glm::scale(glm::mat4(1.0f), Scale);
        }
    };

    struct SpriteRendererComponent
    {
        glm::vec4 Color = {1.0f, 1.0f, 1.0f, 1.0f};
        std::shared_ptr<Texture2D> Texture;
        float TilingFactor = 1.0f;

        SpriteRendererComponent() = default;
        SpriteRendererComponent(const SpriteRendererComponent &) = default;
        SpriteRendererComponent(const glm::vec4 &color) : Color(color) {}
    };
}

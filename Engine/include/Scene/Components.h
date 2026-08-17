#pragma once
#include "Core/UUID.h"
#include "Renderer/Texture.h"
#include <box2d/id.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <string>

namespace Engine
{
    struct IDComponent
    {
        UUID ID;

        IDComponent() = default;
        IDComponent(const IDComponent &) = default;
    };

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

    struct RigidBodyComponent
    {
        enum class BodyType
        {
            Static = 0,
            Dynamic,
            Kinematic
        };

        BodyType Type = BodyType::Static;
        bool FixedRotation = false; // empêche le corps de tourner sous l'effet des collisions

        // Handle Box2D du corps runtime : créé par PhysicsSystem au démarrage du Play,
        // détruit au Stop. Null en édition — ce n'est pas un champ à sérialiser.
        b2BodyId RuntimeBody = b2_nullBodyId;

        RigidBodyComponent() = default;
        RigidBodyComponent(const RigidBodyComponent &) = default;
    };

    struct BoxColliderComponent
    {
        glm::vec2 Offset = {0.0f, 0.0f};
        glm::vec2 Size = {0.5f, 0.5f}; // demi-extensions (convention Box2D : b2MakeBox attend hx/hy)

        float Density = 1.0f;
        float Friction = 0.5f;
        float Restitution = 0.0f;

        // Handle Box2D de la shape runtime, même cycle de vie que RigidBodyComponent::RuntimeBody.
        b2ShapeId RuntimeShape = b2_nullShapeId;

        BoxColliderComponent() = default;
        BoxColliderComponent(const BoxColliderComponent &) = default;
    };

    struct CircleColliderComponent
    {
        glm::vec2 Offset = {0.0f, 0.0f};
        float Radius = 0.5f;

        float Density = 1.0f;
        float Friction = 0.5f;
        float Restitution = 0.0f;

        // Même cycle de vie que BoxColliderComponent::RuntimeShape.
        b2ShapeId RuntimeShape = b2_nullShapeId;

        CircleColliderComponent() = default;
        CircleColliderComponent(const CircleColliderComponent &) = default;
    };
}

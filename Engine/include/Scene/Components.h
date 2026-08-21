#pragma once
#include "Core/UUID.h"
#include "Renderer/Texture.h"
#include "Audio/AudioSource.h"
#include "Renderer/OrthographicCamera.h"
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

    struct AudioComponent
    {
        // Chemin du fichier son, relatif au dossier de travail (pas encore d'asset
        // system : c'est la même limite que SpriteRendererComponent::Texture).
        std::string Path;

        float Volume = 1.0f;
        bool Loop = false;

        // Joué dès le démarrage du Play, façon "Play On Awake" d'Unity.
        bool PlayOnStart = true;

        // Son chargé, créé au Play par AudioSystem et relâché au Stop — comme les
        // handles Box2D, ce n'est pas une donnée à sérialiser.
        std::shared_ptr<AudioSource> Source;

        AudioComponent() = default;
        AudioComponent(const AudioComponent &) = default;
    };

    struct CameraComponent
    {
        // Recalculée à chaque rendu (SetProjection) selon OrthographicSize et le ratio du
        // rendu cible, et repositionnée depuis le TransformComponent de la même entité —
        // ne stocke pas d'état propre entre deux rendus, ce n'est qu'un objet de travail.
        OrthographicCamera Camera{-1.6f, 1.6f, -0.9f, 0.9f};

        float OrthographicSize = 0.9f; // demi-hauteur visible, en unités monde (façon "Size" Unity)

        // La première CameraComponent Primary trouvée dans la scène est celle utilisée
        // pour le rendu (GamePanel). Pas d'unicité forcée si plusieurs sont à true.
        bool Primary = true;

        CameraComponent() = default;
        CameraComponent(const CameraComponent &) = default;
    };
}

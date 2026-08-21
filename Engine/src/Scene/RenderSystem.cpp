#include "Scene/RenderSystem.h"
#include "Scene/Components.h"
#include "Assets/AssetManager.h"
#include "Renderer/Renderer2D.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Engine
{
    void RenderSystem::Render(Scene &scene, const OrthographicCamera &camera)
    {
        Renderer2D::BeginScene(camera);

        auto view = scene.GetAllEntitiesWith<TransformComponent, SpriteRendererComponent>();
        for (auto entityHandle : view)
        {
            auto &sprite = view.get<SpriteRendererComponent>(entityHandle);

            // Transform monde et non local : une entité enfant est placée dans le repère
            // de son parent, le rendu ne connaît que le monde.
            const TransformComponent transform = scene.GetWorldTransform(Entity{entityHandle, &scene});

            // Résolu à chaque frame plutôt que gardé dans le component : un asset
            // rechargé à chaud est ainsi pris en compte sans rien réassigner.
            std::shared_ptr<Texture2D> texture = AssetManager::GetTexture(sprite.Texture);
            if (texture)
                Renderer2D::DrawQuad(transform.GetTransform(), texture, sprite.TilingFactor, sprite.Color);
            else
                Renderer2D::DrawQuad(transform.GetTransform(), sprite.Color);
        }

        Renderer2D::EndScene();
    }
}

namespace Engine
{
    namespace
    {
        // Vert franc, la convention des moteurs 2D pour les contours de collision.
        constexpr glm::vec4 k_ColliderColor = {0.35f, 0.9f, 0.35f, 1.0f};

        glm::mat4 EntityFrame(const TransformComponent &transform)
        {
            // Position + rotation seulement : l'échelle est appliquée séparément, car
            // l'offset d'un collider est exprimé dans le repère du corps sans échelle
            // (c'est ce que fait b2MakeOffsetBox côté PhysicsSystem).
            return glm::translate(glm::mat4(1.0f), transform.Position) *
                   glm::rotate(glm::mat4(1.0f), glm::radians(transform.Rotation), glm::vec3(0.0f, 0.0f, 1.0f));
        }
    }

    namespace
    {
        void DrawColliderOutlinesOf(Entity entity)
        {
            if (!entity.HasComponent<TransformComponent>())
                return;

            // Même règle que pour les sprites : le contour se dessine dans le monde.
            const TransformComponent transform = entity.GetScene()->GetWorldTransform(entity);

            if (entity.HasComponent<BoxColliderComponent>())
            {
                const auto &collider = entity.GetComponent<BoxColliderComponent>();

                // Size est une demi-extension (convention Box2D) et DrawRect attend une
                // taille complète, d'où le facteur 2.
                glm::mat4 outline = EntityFrame(transform) *
                                    glm::translate(glm::mat4(1.0f), glm::vec3(collider.Offset, 0.0f)) *
                                    glm::scale(glm::mat4(1.0f),
                                               {collider.Size.x * transform.Scale.x * 2.0f,
                                                collider.Size.y * transform.Scale.y * 2.0f, 1.0f});
                Renderer2D::DrawRect(outline, k_ColliderColor);
            }

            if (entity.HasComponent<CircleColliderComponent>())
            {
                const auto &collider = entity.GetComponent<CircleColliderComponent>();

                // Même moyenne des échelles que PhysicsSystem : Box2D n'a qu'un rayon, pas d'ellipse.
                const float scale = (transform.Scale.x + transform.Scale.y) * 0.5f;
                const glm::vec3 center = EntityFrame(transform) * glm::vec4(collider.Offset, 0.0f, 1.0f);
                Renderer2D::DrawCircle(center, collider.Radius * scale, k_ColliderColor);
            }
        }
    }

    void RenderSystem::RenderColliderOutlines(Scene &scene, const OrthographicCamera &camera)
    {
        Renderer2D::BeginScene(camera);

        for (auto entityHandle : scene.GetAllEntitiesWith<TransformComponent>())
            DrawColliderOutlinesOf(Entity{entityHandle, &scene});

        Renderer2D::EndScene();
    }

    void RenderSystem::RenderColliderOutline(Entity entity, const OrthographicCamera &camera)
    {
        if (!entity)
            return;

        Renderer2D::BeginScene(camera);
        DrawColliderOutlinesOf(entity);
        Renderer2D::EndScene();
    }
}

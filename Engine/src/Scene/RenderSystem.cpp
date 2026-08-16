#include "Scene/RenderSystem.h"
#include "Scene/Components.h"
#include "Renderer/Renderer2D.h"

namespace Engine
{
    void RenderSystem::Render(Scene &scene, const OrthographicCamera &camera)
    {
        Renderer2D::BeginScene(camera);

        auto view = scene.GetAllEntitiesWith<TransformComponent, SpriteRendererComponent>();
        for (auto entityHandle : view)
        {
            auto [transform, sprite] = view.get<TransformComponent, SpriteRendererComponent>(entityHandle);

            if (sprite.Texture)
                Renderer2D::DrawQuad(transform.GetTransform(), sprite.Texture, sprite.TilingFactor, sprite.Color);
            else
                Renderer2D::DrawQuad(transform.GetTransform(), sprite.Color);
        }

        Renderer2D::EndScene();
    }
}

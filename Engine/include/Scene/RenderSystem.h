#pragma once
#include "Scene/Entity.h"
#include "Scene/Scene.h"
#include "Renderer/OrthographicCamera.h"

namespace Engine
{
    // Itère les entités d'une Scene ayant Transform + SpriteRenderer et les dessine via Renderer2D
    class RenderSystem
    {
    public:
        static void Render(Scene &scene, const OrthographicCamera &camera);

        // Contours des colliders, superposés à la scène (outil d'édition : les colliders
        // sont invisibles autrement, un CircleCollider et un BoxCollider se ressemblant
        // trait pour trait à l'écran puisque le sprite est toujours un quad).
        // Reproduit exactement la géométrie envoyée à Box2D par PhysicsSystem.
        static void RenderColliderOutlines(Scene &scene, const OrthographicCamera &camera);

        // Contour des seuls colliders d'une entité : l'éditeur l'affiche en permanence
        // pour l'entité sélectionnée, comme Unity le fait de ses gizmos de collision.
        static void RenderColliderOutline(Entity entity, const OrthographicCamera &camera);
    };
}

#pragma once
#include "Scene/Scene.h"
#include "Renderer/OrthographicCamera.h"

namespace Engine
{
    // Itère les entités d'une Scene ayant Transform + SpriteRenderer et les dessine via Renderer2D
    class RenderSystem
    {
    public:
        static void Render(Scene &scene, const OrthographicCamera &camera);
    };
}

#pragma once
#include <entt/entt.hpp>
#include <memory>
#include <string>

namespace Engine
{
    class Entity;

    class Scene
    {
    public:
        Scene() = default;
        ~Scene() = default;

        Entity CreateEntity(const std::string &name = std::string());
        void DestroyEntity(Entity entity);

        // Copie profonde de toutes les entités/components (préserve les UUID et les
        // références de texture) — utilisé pour créer la scène de "jeu" au moment du Play,
        // sans jamais toucher à la scène d'édition originale.
        std::shared_ptr<Scene> Copy();

        template <typename... Components>
        auto GetAllEntitiesWith()
        {
            return m_Registry.view<Components...>();
        }

    private:
        entt::registry m_Registry;

        friend class Entity;
    };
}

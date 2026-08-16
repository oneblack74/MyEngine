#pragma once
#include <entt/entt.hpp>
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

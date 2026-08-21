#pragma once
#include "Scene/Scene.h"
#include "Scene/Components.h"
#include <entt/entt.hpp>
#include <cassert>
#include <utility>

namespace Engine
{
    class Entity
    {
    public:
        Entity() = default;
        Entity(entt::entity handle, Scene *scene);
        Entity(const Entity &other) = default;

        template <typename T, typename... Args>
        T &AddComponent(Args &&...args)
        {
            assert(!HasComponent<T>() && "L'entité a déjà ce component !");
            return m_Scene->m_Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
        }

        template <typename T>
        T &GetComponent()
        {
            assert(HasComponent<T>() && "L'entité n'a pas ce component !");
            return m_Scene->m_Registry.get<T>(m_EntityHandle);
        }

        template <typename T>
        bool HasComponent()
        {
            return m_Scene->m_Registry.all_of<T>(m_EntityHandle);
        }

        template <typename T>
        void RemoveComponent()
        {
            assert(HasComponent<T>() && "L'entité n'a pas ce component !");
            m_Scene->m_Registry.remove<T>(m_EntityHandle);
        }

        UUID GetUUID() { return GetComponent<IDComponent>().ID; }
        const std::string &GetName() { return GetComponent<TagComponent>().Tag; }

        // La scène propriétaire — nécessaire dès qu'on veut interroger la hiérarchie
        // (parent, enfants, transform monde) à partir d'une entité seule.
        Scene *GetScene() const { return m_Scene; }

        operator bool() const { return m_EntityHandle != entt::null; }
        operator entt::entity() const { return m_EntityHandle; }
        operator uint32_t() const { return (uint32_t)m_EntityHandle; }

        bool operator==(const Entity &other) const
        {
            return m_EntityHandle == other.m_EntityHandle && m_Scene == other.m_Scene;
        }
        bool operator!=(const Entity &other) const { return !(*this == other); }

    private:
        entt::entity m_EntityHandle{entt::null};
        Scene *m_Scene = nullptr;
    };
}

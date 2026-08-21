#include "Scene/Scene.h"
#include "Scene/Entity.h"
#include "Scene/Components.h"
#include <algorithm>

namespace Engine
{
    namespace
    {
        template <typename T>
        void CopyComponentIfPresent(Entity source, Entity destination)
        {
            if (!source.HasComponent<T>())
                return;

            if (destination.HasComponent<T>())
                destination.GetComponent<T>() = source.GetComponent<T>();
            else
                destination.AddComponent<T>(source.GetComponent<T>());
        }
    }

    Entity Scene::CreateEntity(const std::string &name)
    {
        return CreateEntityWithUUID(UUID(), name);
    }

    Entity Scene::CreateEntityWithUUID(UUID uuid, const std::string &name)
    {
        Entity entity = {m_Registry.create(), this};
        entity.AddComponent<IDComponent>().ID = uuid;
        entity.AddComponent<TransformComponent>();
        auto &tag = entity.AddComponent<TagComponent>();
        tag.Tag = name.empty() ? "Entity" : name;

        m_EntityMap[uuid] = entity;
        m_EntityOrder.push_back(uuid);
        return entity;
    }

    void Scene::DestroyEntity(Entity entity)
    {
        const UUID uuid = entity.GetComponent<IDComponent>().ID;
        m_EntityMap.erase(uuid);
        m_EntityOrder.erase(std::remove(m_EntityOrder.begin(), m_EntityOrder.end(), uuid),
                            m_EntityOrder.end());

        m_Registry.destroy(entity);
    }

    size_t Scene::GetEntityOrderIndex(UUID uuid) const
    {
        auto it = std::find(m_EntityOrder.begin(), m_EntityOrder.end(), uuid);
        return (size_t)std::distance(m_EntityOrder.begin(), it);
    }

    void Scene::SetEntityOrderIndex(UUID uuid, size_t index)
    {
        auto it = std::find(m_EntityOrder.begin(), m_EntityOrder.end(), uuid);
        if (it == m_EntityOrder.end())
            return;

        m_EntityOrder.erase(it);
        m_EntityOrder.insert(m_EntityOrder.begin() + (long)std::min(index, m_EntityOrder.size()), uuid);
    }

    Entity Scene::FindEntityByUUID(UUID uuid)
    {
        auto it = m_EntityMap.find(uuid);
        if (it == m_EntityMap.end())
            return {};

        return Entity{it->second, this};
    }

    Entity Scene::DuplicateEntity(Entity source)
    {
        Entity copy = CreateEntity(source.GetComponent<TagComponent>().Tag);
        CopyComponents(source, copy);
        return copy;
    }

    void Scene::CopyComponents(Entity source, Entity destination)
    {
        destination.GetComponent<TagComponent>().Tag = source.GetComponent<TagComponent>().Tag;

        if (source.HasComponent<TransformComponent>())
            destination.GetComponent<TransformComponent>() = source.GetComponent<TransformComponent>();

        // Chaque component est copié tel quel, textures comprises (shared_ptr partagé).
        // RuntimeBody/RuntimeShape font exception de fait : ils sont nuls dans une scène
        // d'édition, et c'est PhysicsSystem qui les recrée au Play pour la scène runtime.
        CopyComponentIfPresent<SpriteRendererComponent>(source, destination);
        CopyComponentIfPresent<RigidBodyComponent>(source, destination);
        CopyComponentIfPresent<BoxColliderComponent>(source, destination);
        CopyComponentIfPresent<CircleColliderComponent>(source, destination);
        CopyComponentIfPresent<AudioComponent>(source, destination);
        CopyComponentIfPresent<CameraComponent>(source, destination);
    }

    std::shared_ptr<Scene> Scene::Copy()
    {
        auto newScene = std::make_shared<Scene>();

        // Parcours dans l'ordre de la hiérarchie, pas dans celui du registre : la copie
        // doit présenter ses entités exactement comme l'originale.
        for (UUID uuid : m_EntityOrder)
        {
            Entity srcEntity = FindEntityByUUID(uuid);
            Entity newEntity = newScene->CreateEntityWithUUID(uuid,
                                                              srcEntity.GetComponent<TagComponent>().Tag);
            CopyComponents(srcEntity, newEntity);
        }

        return newScene;
    }
}

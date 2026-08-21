#include "Scene/Scene.h"
#include "Scene/Entity.h"
#include "Scene/Components.h"

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
        return entity;
    }

    void Scene::DestroyEntity(Entity entity)
    {
        m_Registry.destroy(entity);
    }

    Entity Scene::FindEntityByUUID(UUID uuid)
    {
        auto view = GetAllEntitiesWith<IDComponent>();
        for (auto entityHandle : view)
        {
            if (m_Registry.get<IDComponent>(entityHandle).ID == uuid)
                return Entity{entityHandle, this};
        }
        return {};
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

        auto view = GetAllEntitiesWith<IDComponent>();
        for (auto entityHandle : view)
        {
            Entity srcEntity{entityHandle, this};
            Entity newEntity = newScene->CreateEntityWithUUID(srcEntity.GetComponent<IDComponent>().ID,
                                                              srcEntity.GetComponent<TagComponent>().Tag);
            CopyComponents(srcEntity, newEntity);
        }

        return newScene;
    }
}

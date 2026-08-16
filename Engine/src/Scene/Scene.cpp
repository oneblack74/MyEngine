#include "Scene/Scene.h"
#include "Scene/Entity.h"
#include "Scene/Components.h"

namespace Engine
{
    Entity Scene::CreateEntity(const std::string &name)
    {
        Entity entity = {m_Registry.create(), this};
        entity.AddComponent<IDComponent>(); // UUID généré automatiquement (constructeur par défaut de UUID)
        entity.AddComponent<TransformComponent>();
        auto &tag = entity.AddComponent<TagComponent>();
        tag.Tag = name.empty() ? "Entity" : name;
        return entity;
    }

    void Scene::DestroyEntity(Entity entity)
    {
        m_Registry.destroy(entity);
    }

    std::shared_ptr<Scene> Scene::Copy()
    {
        auto newScene = std::make_shared<Scene>();

        auto view = GetAllEntitiesWith<IDComponent>();
        for (auto entityHandle : view)
        {
            Entity srcEntity{entityHandle, this};
            UUID uuid = srcEntity.GetComponent<IDComponent>().ID;
            const std::string &name = srcEntity.GetComponent<TagComponent>().Tag;

            Entity newEntity = newScene->CreateEntity(name);
            newEntity.GetComponent<IDComponent>().ID = uuid;

            if (srcEntity.HasComponent<TransformComponent>())
                newEntity.GetComponent<TransformComponent>() = srcEntity.GetComponent<TransformComponent>();

            if (srcEntity.HasComponent<SpriteRendererComponent>())
                newEntity.AddComponent<SpriteRendererComponent>(srcEntity.GetComponent<SpriteRendererComponent>());
        }

        return newScene;
    }
}

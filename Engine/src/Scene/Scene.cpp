#include "Scene/Scene.h"
#include "Scene/Entity.h"
#include "Scene/Components.h"

namespace Engine
{
    Entity Scene::CreateEntity(const std::string &name)
    {
        Entity entity = {m_Registry.create(), this};
        entity.AddComponent<TransformComponent>();
        auto &tag = entity.AddComponent<TagComponent>();
        tag.Tag = name.empty() ? "Entity" : name;
        return entity;
    }

    void Scene::DestroyEntity(Entity entity)
    {
        m_Registry.destroy(entity);
    }
}

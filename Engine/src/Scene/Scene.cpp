#include "Scene/Scene.h"
#include "Scene/Entity.h"
#include "Scene/Components.h"
#include <algorithm>
#include <cmath>

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

    namespace
    {
        // Une échelle nulle rendrait le transform local indéfini (division par zéro) :
        // on la ramène à une valeur négligeable plutôt que de produire des NaN.
        float NonZero(float value)
        {
            return std::fabs(value) < 1e-6f ? 1e-6f : value;
        }

        // Composition de deux transforms 2D, écrite en position/rotation/échelle plutôt
        // qu'en matrices : le moteur n'a qu'une rotation (autour de Z) et cette forme
        // s'inverse directement, ce dont SetWorldTransform a besoin.
        TransformComponent Combine(const TransformComponent &parent, const TransformComponent &local)
        {
            const float cosR = std::cos(glm::radians(parent.Rotation));
            const float sinR = std::sin(glm::radians(parent.Rotation));
            const glm::vec3 scaled = local.Position * parent.Scale;

            TransformComponent world;
            world.Position = {parent.Position.x + scaled.x * cosR - scaled.y * sinR,
                              parent.Position.y + scaled.x * sinR + scaled.y * cosR,
                              parent.Position.z + scaled.z};
            world.Rotation = parent.Rotation + local.Rotation;
            world.Scale = parent.Scale * local.Scale;
            return world;
        }

        TransformComponent Separate(const TransformComponent &parent, const TransformComponent &world)
        {
            const float cosR = std::cos(glm::radians(-parent.Rotation));
            const float sinR = std::sin(glm::radians(-parent.Rotation));
            const glm::vec3 delta = world.Position - parent.Position;
            const glm::vec3 safeScale = {NonZero(parent.Scale.x), NonZero(parent.Scale.y),
                                         NonZero(parent.Scale.z)};

            TransformComponent local;
            local.Position = glm::vec3{delta.x * cosR - delta.y * sinR,
                                       delta.x * sinR + delta.y * cosR,
                                       delta.z} /
                             safeScale;
            local.Rotation = world.Rotation - parent.Rotation;
            local.Scale = world.Scale / safeScale;
            return local;
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
        entity.AddComponent<ParentComponent>();
        auto &tag = entity.AddComponent<TagComponent>();
        tag.Tag = name.empty() ? "Entity" : name;

        m_EntityMap[uuid] = entity;
        m_EntityOrder.push_back(uuid);
        return entity;
    }

    void Scene::DestroyEntity(Entity entity)
    {
        // Les enfants disparaissent avec leur parent, comme dans Unity. GetChildren
        // renvoie une copie : la liste n'est pas invalidée par les suppressions.
        for (Entity child : GetChildren(entity))
            DestroyEntity(child);

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

    Entity Scene::GetParent(Entity entity)
    {
        if (!entity || !entity.HasComponent<ParentComponent>())
            return {};

        // Un Parent nul ne correspond à aucune entité : FindEntityByUUID renvoie alors
        // l'entité vide, qui est exactement ce que veut dire "à la racine".
        return FindEntityByUUID(entity.GetComponent<ParentComponent>().Parent);
    }

    std::vector<Entity> Scene::GetChildren(Entity entity)
    {
        if (!entity)
            return GetRootEntities();

        const UUID parentID = entity.GetComponent<IDComponent>().ID;

        std::vector<Entity> children;
        for (UUID uuid : m_EntityOrder)
        {
            Entity candidate = FindEntityByUUID(uuid);
            if (candidate && candidate.HasComponent<ParentComponent>() &&
                candidate.GetComponent<ParentComponent>().Parent == parentID)
            {
                children.push_back(candidate);
            }
        }
        return children;
    }

    Entity Scene::GetRootEntity()
    {
        std::vector<Entity> roots = GetRootEntities();
        return roots.empty() ? Entity{} : roots.front();
    }

    Entity Scene::EnsureSingleRoot(const std::string &rootName)
    {
        std::vector<Entity> roots = GetRootEntities();
        if (roots.size() == 1)
            return roots.front();

        // La nouvelle racine est créée puis remontée en tête de l'ordre d'affichage :
        // elle doit apparaître avant les entités qu'elle coiffe.
        Entity root = CreateEntity(rootName);
        for (Entity previousRoot : roots)
            SetParent(previousRoot, root);

        MoveEntityBefore(root.GetComponent<IDComponent>().ID, m_EntityOrder.front());
        return root;
    }

    std::vector<Entity> Scene::GetRootEntities()
    {
        std::vector<Entity> roots;
        for (UUID uuid : m_EntityOrder)
        {
            Entity candidate = FindEntityByUUID(uuid);
            if (candidate && !GetParent(candidate))
                roots.push_back(candidate);
        }
        return roots;
    }

    bool Scene::IsDescendantOf(Entity entity, Entity possibleAncestor)
    {
        for (Entity current = GetParent(entity); current; current = GetParent(current))
        {
            if (current == possibleAncestor)
                return true;
        }
        return false;
    }

    bool Scene::CanSetParent(Entity child, Entity parent)
    {
        if (!child || !child.HasComponent<ParentComponent>())
            return false;

        // Rattacher une entité à elle-même ou à l'un de ses propres descendants
        // détacherait la branche du reste de la scène et boucherait tous les parcours.
        return !parent || (child != parent && !IsDescendantOf(parent, child));
    }

    bool Scene::SetParent(Entity child, Entity parent)
    {
        if (!CanSetParent(child, parent))
            return false;

        // La position dans le monde est conservée : seul le transform local change,
        // comme le fait Unity par défaut.
        const TransformComponent world = GetWorldTransform(child);
        child.GetComponent<ParentComponent>().Parent = parent ? parent.GetComponent<IDComponent>().ID : UUID(0);
        SetWorldTransform(child, world);
        return true;
    }

    TransformComponent Scene::GetWorldTransform(Entity entity)
    {
        if (!entity || !entity.HasComponent<TransformComponent>())
            return {};

        const TransformComponent local = entity.GetComponent<TransformComponent>();
        Entity parent = GetParent(entity);
        if (!parent)
            return local;

        return Combine(GetWorldTransform(parent), local);
    }

    void Scene::SetWorldTransform(Entity entity, const TransformComponent &world)
    {
        if (!entity || !entity.HasComponent<TransformComponent>())
            return;

        Entity parent = GetParent(entity);
        entity.GetComponent<TransformComponent>() =
            parent ? Separate(GetWorldTransform(parent), world) : world;
    }

    void Scene::MoveEntityBefore(UUID moved, UUID reference)
    {
        auto movedIt = std::find(m_EntityOrder.begin(), m_EntityOrder.end(), moved);
        if (movedIt == m_EntityOrder.end())
            return;

        m_EntityOrder.erase(movedIt);

        auto referenceIt = std::find(m_EntityOrder.begin(), m_EntityOrder.end(), reference);
        m_EntityOrder.insert(referenceIt, moved);
    }

    void Scene::MoveEntityToEnd(UUID moved)
    {
        auto movedIt = std::find(m_EntityOrder.begin(), m_EntityOrder.end(), moved);
        if (movedIt == m_EntityOrder.end())
            return;

        m_EntityOrder.erase(movedIt);
        m_EntityOrder.push_back(moved);
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

        // Le lien de parenté ne suit que si le parent existe aussi côté destination :
        // coller une entité dont le parent a disparu la pose à la racine plutôt que de
        // la rendre inaccessible derrière un UUID mort.
        if (source.HasComponent<ParentComponent>() && destination.HasComponent<ParentComponent>())
        {
            const UUID parent = source.GetComponent<ParentComponent>().Parent;
            Scene *destinationScene = destination.GetScene();
            const bool parentExists = destinationScene && destinationScene->FindEntityByUUID(parent);
            destination.GetComponent<ParentComponent>().Parent = parentExists ? parent : UUID(0);
        }

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
        //
        // Deux passes : toutes les entités existent avant qu'on copie le moindre
        // component, sinon un parent créé après son enfant serait vu comme absent et le
        // lien de parenté serait perdu.
        for (UUID uuid : m_EntityOrder)
            newScene->CreateEntityWithUUID(uuid, FindEntityByUUID(uuid).GetComponent<TagComponent>().Tag);

        for (UUID uuid : m_EntityOrder)
            CopyComponents(FindEntityByUUID(uuid), newScene->FindEntityByUUID(uuid));

        return newScene;
    }
}

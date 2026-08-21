#include "SceneInstanceSync.h"
#include <Scene/Components.h>
#include <Scene/Entity.h>
#include <Scene/SceneSerializer.h>
#include <functional>
#include <vector>

using json = nlohmann::json;

namespace
{
    // Clés qui n'ont pas à être fusionnées : l'identité de l'entité, sa place dans la
    // scène qui l'accueille, et les liens d'instanciation eux-mêmes. Les UUID de la
    // source ne veulent rien dire du côté de l'instance.
    bool IsInstanceOwnedKey(const std::string &key)
    {
        return key == "UUID" || key == "Parent" || key == "SceneInstanceComponent" ||
               key == "SceneInstanceMemberComponent";
    }

    // Index des entités d'une scène sérialisée, par UUID.
    std::unordered_map<uint64_t, const json *> IndexByUUID(const json &sceneJson)
    {
        std::unordered_map<uint64_t, const json *> index;
        if (!sceneJson.contains("Entities"))
            return index;

        for (const json &entityJson : sceneJson["Entities"])
        {
            if (entityJson.contains("UUID"))
                index[entityJson["UUID"].get<uint64_t>()] = &entityJson;
        }
        return index;
    }

    // Fusion d'un component : champ par champ, la valeur de l'instance l'emporte si elle
    // s'écarte de la base, sinon la nouvelle valeur de la source passe.
    json MergeComponent(const json &current, const json *base, const json &updated)
    {
        json merged = updated;

        for (auto it = current.begin(); it != current.end(); ++it)
        {
            const std::string &field = it.key();
            const bool inBase = base && base->contains(field);

            // Champ ajouté sur l'instance : rien à comparer, on le garde.
            if (!inBase)
            {
                merged[field] = it.value();
                continue;
            }

            // Champ modifié sur l'instance depuis la dernière synchronisation : c'est
            // une surcharge, elle survit à la mise à jour de la source.
            if (it.value() != (*base)[field])
                merged[field] = it.value();
        }

        return merged;
    }

    json MergeEntity(const json &current, const json *base, const json &updated)
    {
        json merged = current;

        // Ce que la source apporte ou met à jour.
        for (auto it = updated.begin(); it != updated.end(); ++it)
        {
            const std::string &key = it.key();
            if (IsInstanceOwnedKey(key))
                continue;

            if (!current.contains(key))
            {
                // Component absent de l'instance : ajouté par la source (ou retiré sur
                // l'instance, auquel cas il revient — le distinguer demanderait de
                // retenir les suppressions, ce qui n'est pas fait).
                merged[key] = it.value();
                continue;
            }

            const json *baseComponent = (base && base->contains(key)) ? &(*base)[key] : nullptr;
            merged[key] = it.value().is_object()
                              ? MergeComponent(current[key], baseComponent, it.value())
                              : (baseComponent && current[key] != *baseComponent ? current[key] : it.value());
        }

        // Ce que la source a retiré depuis la base.
        std::vector<std::string> removed;
        for (auto it = current.begin(); it != current.end(); ++it)
        {
            const std::string &key = it.key();
            if (IsInstanceOwnedKey(key) || updated.contains(key))
                continue;

            // Présent dans la base mais plus dans la source : supprimé côté source. S'il
            // n'était pas dans la base non plus, c'est un ajout propre à l'instance,
            // qu'on garde.
            if (base && base->contains(key))
                removed.push_back(key);
        }
        for (const std::string &key : removed)
            merged.erase(key);

        return merged;
    }
}

void SceneInstanceSync::RememberSource(Engine::AssetHandle source, const json &sourceJson)
{
    m_Bases[(uint64_t)source] = sourceJson;
}

bool SceneInstanceSync::KnowsSource(Engine::AssetHandle source) const
{
    return m_Bases.find((uint64_t)source) != m_Bases.end();
}

bool SceneInstanceSync::Refresh(const std::shared_ptr<Engine::Scene> &sceneHolder,
                                Engine::AssetHandle source, const json &newSourceJson)
{
    Engine::Scene &scene = *sceneHolder;
    // Sans base connue, la nouvelle version fait office de référence : tout ce qui
    // diffère côté instance est alors considéré comme une surcharge, ce qui est le choix
    // prudent — on ne détruit rien.
    auto baseIt = m_Bases.find((uint64_t)source);
    const json &baseJson = baseIt != m_Bases.end() ? baseIt->second : newSourceJson;

    const auto baseIndex = IndexByUUID(baseJson);
    const auto updatedIndex = IndexByUUID(newSourceJson);

    // Les entités appartenant à une instance de cette source, repérées par l'entité
    // dont elles sont issues.
    std::unordered_map<uint64_t, uint64_t> memberSourceOf; // UUID de l'entité -> UUID côté source
    for (auto entityHandle : scene.GetAllEntitiesWith<Engine::SceneInstanceMemberComponent>())
    {
        Engine::Entity entity{entityHandle, &scene};

        // Remonter jusqu'à la racine d'instance pour savoir de quelle source il s'agit.
        Engine::Entity root = entity;
        while (root && !root.HasComponent<Engine::SceneInstanceComponent>())
            root = scene.GetParent(root);

        if (!root || (uint64_t)root.GetComponent<Engine::SceneInstanceComponent>().Source != (uint64_t)source)
            continue;

        memberSourceOf[(uint64_t)entity.GetUUID()] =
            (uint64_t)entity.GetComponent<Engine::SceneInstanceMemberComponent>().SourceEntity;
    }

    if (memberSourceOf.empty())
        return false;

    json merged = Engine::SceneSerializer(sceneHolder).ToJson();

    for (json &entityJson : merged["Entities"])
    {
        auto member = memberSourceOf.find(entityJson["UUID"].get<uint64_t>());
        if (member == memberSourceOf.end())
            continue;

        auto updated = updatedIndex.find(member->second);
        if (updated == updatedIndex.end())
            continue; // entité disparue de la source : traitée après, côté structure

        auto base = baseIndex.find(member->second);
        entityJson = MergeEntity(entityJson, base != baseIndex.end() ? base->second : nullptr,
                                 *updated->second);
    }

    if (!merged.contains("Entities"))
        return false;

    // Relecture en place : le même objet Scene, vidé puis rempli depuis le JSON fusionné.
    scene.Clear();
    if (!Engine::SceneSerializer(sceneHolder).FromJson(merged))
        return false;

    ApplyStructuralChanges(sceneHolder, source, newSourceJson);
    return true;
}

void SceneInstanceSync::ApplyStructuralChanges(const std::shared_ptr<Engine::Scene> &scene,
                                               Engine::AssetHandle source, const json &newSourceJson)
{
    // La source est relue en scène : suivre ses parents et ses enfants est bien plus
    // simple sur l'arbre que sur le JSON.
    auto updatedScene = std::make_shared<Engine::Scene>();
    if (!Engine::SceneSerializer(updatedScene).FromJson(newSourceJson))
        return;

    Engine::Entity updatedRoot = updatedScene->GetRootEntity();
    if (!updatedRoot)
        return;

    std::vector<Engine::UUID> instanceRoots;
    for (auto entityHandle : scene->GetAllEntitiesWith<Engine::SceneInstanceComponent>())
    {
        Engine::Entity entity{entityHandle, scene.get()};
        if ((uint64_t)entity.GetComponent<Engine::SceneInstanceComponent>().Source == (uint64_t)source)
            instanceRoots.push_back(entity.GetUUID());
    }

    for (Engine::UUID rootID : instanceRoots)
    {
        Engine::Entity instanceRoot = scene->FindEntityByUUID(rootID);
        if (!instanceRoot)
            continue;

        // Membres actuels de l'instance, indexés par l'entité de la source dont ils
        // sont issus.
        std::unordered_map<uint64_t, Engine::Entity> memberOf;
        std::function<void(Engine::Entity)> collect = [&](Engine::Entity entity)
        {
            if (entity.HasComponent<Engine::SceneInstanceMemberComponent>())
            {
                memberOf[(uint64_t)entity.GetComponent<Engine::SceneInstanceMemberComponent>().SourceEntity] =
                    entity;
            }
            for (Engine::Entity child : scene->GetChildren(entity))
                collect(child);
        };
        collect(instanceRoot);

        // Entités disparues de la source : elles s'en vont de l'instance aussi.
        std::vector<Engine::Entity> obsolete;
        for (const auto &[sourceID, member] : memberOf)
        {
            if (!updatedScene->FindEntityByUUID(Engine::UUID(sourceID)))
                obsolete.push_back(member);
        }
        for (Engine::Entity member : obsolete)
        {
            if (member != instanceRoot)
                scene->DestroyEntity(member);
        }

        // Entités ajoutées dans la source : instanciées sous le membre correspondant à
        // leur parent. InstantiateBranch emmène toute la descendance, d'où l'arrêt dès
        // qu'une entité manquante est trouvée.
        std::function<void(Engine::Entity)> addMissing = [&](Engine::Entity sourceEntity)
        {
            auto existing = memberOf.find((uint64_t)sourceEntity.GetUUID());
            if (existing == memberOf.end())
            {
                Engine::Entity sourceParent = updatedScene->GetParent(sourceEntity);
                Engine::Entity parent = instanceRoot;
                if (sourceParent && sourceParent != updatedRoot)
                {
                    auto parentMember = memberOf.find((uint64_t)sourceParent.GetUUID());
                    if (parentMember == memberOf.end())
                        return; // parent lui-même absent : il sera traité avec sa branche
                    parent = parentMember->second;
                }

                scene->InstantiateBranch(sourceEntity, parent);
                return;
            }

            for (Engine::Entity child : updatedScene->GetChildren(sourceEntity))
                addMissing(child);
        };

        for (Engine::Entity child : updatedScene->GetChildren(updatedRoot))
            addMissing(child);
    }
}

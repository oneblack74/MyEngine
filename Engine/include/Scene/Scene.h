#pragma once
#include "Core/UUID.h"
#include <entt/entt.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Engine
{
    class Entity;

    class Scene
    {
    public:
        Scene() = default;
        ~Scene() = default;

        Entity CreateEntity(const std::string &name = std::string());
        // Recrée une entité avec un UUID imposé — nécessaire pour annuler une suppression :
        // l'entité doit revenir avec la même identité, sans quoi toutes les références
        // (sélection, commandes d'annulation en attente) pointeraient dans le vide.
        Entity CreateEntityWithUUID(UUID uuid, const std::string &name = std::string());
        void DestroyEntity(Entity entity);

        // Entité nulle si l'UUID est inconnu. Les handles entt sont recyclés et changent
        // à chaque copie de scène : l'UUID est la seule référence stable.
        Entity FindEntityByUUID(UUID uuid);

        // Copie l'entité dans cette même scène, avec un nouvel UUID (duplication éditeur).
        Entity DuplicateEntity(Entity source);

        // Copie tous les components de source vers destination, y compris entre deux
        // scènes différentes. L'IDComponent n'est pas touché : c'est l'identité de la
        // destination, pas une donnée à recopier.
        static void CopyComponents(Entity source, Entity destination);

        // Copie profonde de toutes les entités/components (préserve les UUID et les
        // références de texture) — utilisé pour créer la scène de "jeu" au moment du Play,
        // sans jamais toucher à la scène d'édition originale.
        std::shared_ptr<Scene> Copy();

        // Ordre d'affichage des entités, celui de la hiérarchie. Une vue EnTT n'a pas
        // d'ordre garanti et le registre remanie son stockage à chaque suppression
        // (swap-and-pop) : sans cette liste, les entités changeraient de place toutes
        // seules dès qu'on en supprime une.
        const std::vector<UUID> &GetEntityOrder() const { return m_EntityOrder; }

        // Position d'une entité dans cet ordre, ou la taille de la liste si elle est
        // inconnue.
        size_t GetEntityOrderIndex(UUID uuid) const;

        // Replace une entité à une position donnée — sert à annuler une suppression
        // sans que l'entité ressorte tout en bas de la hiérarchie.
        void SetEntityOrderIndex(UUID uuid, size_t index);

        template <typename... Components>
        auto GetAllEntitiesWith()
        {
            return m_Registry.view<Components...>();
        }

    private:
        entt::registry m_Registry;

        // Les handles EnTT sont recyclés et changent à chaque copie de scène : l'UUID
        // est la seule clé stable, d'où cette table pour retrouver une entité sans
        // parcourir tout le registre.
        std::unordered_map<UUID, entt::entity> m_EntityMap;
        std::vector<UUID> m_EntityOrder;

        friend class Entity;
    };
}

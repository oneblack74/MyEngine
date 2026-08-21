#pragma once
#include "Core/UUID.h"
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

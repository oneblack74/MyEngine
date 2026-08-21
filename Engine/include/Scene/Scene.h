#pragma once
#include "Core/UUID.h"
// Pour TransformComponent, renvoyé par valeur par GetWorldTransform().
#include "Scene/Components.h"
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

        // Recopie une branche venue d'une autre scène sous `parent`, avec de nouveaux
        // UUID : une instance est une copie indépendante, deux instances d'une même
        // scène ne peuvent pas partager l'identité de leurs entités. Les transforms
        // locaux sont conservés tels qu'ils ont été composés dans la source.
        Entity InstantiateBranch(Entity source, Entity parent);

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

        // Une scène de jeu a exactement une entité racine — c'est ce qui permet de
        // l'instancier ailleurs comme un objet, façon Godot. La classe Scene elle-même
        // n'impose rien : elle sert aussi de conteneur détaché (presse-papiers,
        // sauvegarde d'annulation), où cette notion n'a pas de sens. L'invariant est
        // tenu par SceneManager, SceneSerializer et l'éditeur.
        Entity GetRootEntity();

        // Regroupe toutes les entités racine sous une nouvelle entité, et renvoie
        // celle-ci. Sans effet s'il y a déjà exactement une racine.
        Entity EnsureSingleRoot(const std::string &rootName);

        // Entités sans parent, dans l'ordre d'affichage.
        std::vector<Entity> GetRootEntities();
        std::vector<Entity> GetChildren(Entity entity);
        Entity GetParent(Entity entity);
        bool IsDescendantOf(Entity entity, Entity possibleAncestor);

        // Un rattachement est refusé s'il crée un cycle : une entité ne peut devenir
        // ni son propre parent, ni l'enfant de l'un de ses descendants.
        bool CanSetParent(Entity child, Entity parent);

        // Rattache child à parent (parent nul = racine). La position dans le monde est
        // conservée : c'est le transform local qui est recalculé. Renvoie false si le
        // rattachement créerait un cycle, auquel cas rien n'est modifié.
        bool SetParent(Entity child, Entity parent);

        // Le TransformComponent est exprimé dans le repère du parent. Ces deux fonctions
        // font le passage avec le repère du monde — le seul que comprennent le rendu, la
        // physique et les gizmos.
        TransformComponent GetWorldTransform(Entity entity);
        void SetWorldTransform(Entity entity, const TransformComponent &world);

        // Position d'une entité dans cet ordre, ou la taille de la liste si elle est
        // inconnue.
        size_t GetEntityOrderIndex(UUID uuid) const;

        // Déplacent une entité dans l'ordre d'affichage : juste avant une autre, ou en
        // dernier. L'ordre est global et plat ; seule compte la position relative des
        // entités qui partagent un même parent.
        void MoveEntityBefore(UUID moved, UUID reference);
        void MoveEntityToEnd(UUID moved);

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

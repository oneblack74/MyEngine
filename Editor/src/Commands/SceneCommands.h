#pragma once
#include <vector>
#include "Commands/EditorCommand.h"
#include "Commands/EditorContext.h"
#include <Core/UUID.h>
#include <Scene/Components.h>
#include <memory>
#include <string>

// Édition d'un component depuis l'Inspecteur ou le gizmo : on mémorise la valeur
// complète du component avant et après, et annuler/rétablir revient à réécrire l'une
// ou l'autre. Copier tout le component plutôt que le champ modifié garde la commande
// unique quel que soit le widget, au prix de quelques dizaines d'octets.
template <typename T>
class ComponentEditCommand : public EditorCommand
{
public:
    ComponentEditCommand(EditorContext &context, Engine::UUID entityID,
                         const T &before, const T &after, std::string name)
        : m_Context(context), m_EntityID(entityID), m_Before(before), m_After(after),
          m_Name(std::move(name)) {}

    void Redo() override { Apply(m_After); }
    void Undo() override { Apply(m_Before); }
    std::string GetName() const override { return m_Name; }

private:
    void Apply(const T &value)
    {
        Engine::Entity entity = m_Context.GetEditorScene().FindEntityByUUID(m_EntityID);
        if (!entity || !entity.HasComponent<T>())
            return;

        entity.GetComponent<T>() = value;
        m_Context.SelectEntity(entity);
    }

    EditorContext &m_Context;
    Engine::UUID m_EntityID;
    T m_Before;
    T m_After;
    std::string m_Name;
};

// Renommage : le Tag est une chaîne, pas un component copiable comme les autres.
class RenameEntityCommand : public EditorCommand
{
public:
    RenameEntityCommand(EditorContext &context, Engine::UUID entityID,
                        std::string before, std::string after);

    void Redo() override;
    void Undo() override;
    std::string GetName() const override;

private:
    void Apply(const std::string &name);

    EditorContext &m_Context;
    Engine::UUID m_EntityID;
    std::string m_Before;
    std::string m_After;
};

// Création, suppression, duplication et collage partagent le même mécanisme : une
// entité de sauvegarde vit dans une scène détachée, qui ne sert qu'à conserver ses
// components. Ça évite d'énumérer les types de components ici — Scene::CopyComponents
// le fait déjà, et une commande n'a pas à être mise à jour à chaque nouveau component.
class CreateEntityCommand : public EditorCommand
{
public:
    CreateEntityCommand(EditorContext &context, std::string name);

    void Redo() override;
    void Undo() override;
    std::string GetName() const override;

protected:
    // L'UUID est décidé une fois pour toutes à la construction : un rétablissement doit
    // faire revenir la même entité, pas une nouvelle.
    EditorContext &m_Context;
    Engine::UUID m_EntityID;
    std::string m_EntityName;

    // Vide pour une création simple ; renseignée pour une duplication ou un collage.
    std::shared_ptr<Engine::Scene> m_Source;
    Engine::Entity m_SourceEntity;
};

// Duplication (Ctrl+D) et collage (Ctrl+V) : même chose qu'une création, mais l'entité
// naît avec les components d'un modèle.
class CreateEntityFromCommand : public CreateEntityCommand
{
public:
    CreateEntityFromCommand(EditorContext &context, Engine::Entity source, std::string actionName);

    std::string GetName() const override;

private:
    std::string m_ActionName;
};

// Rattachement et réordonnancement, tous deux issus d'un glisser-déposer dans la
// hiérarchie : c'est la même opération à un détail près, la position finale.
class ReparentEntityCommand : public EditorCommand
{
public:
    // newParent nul = racine. insertBefore nul = placer en dernier parmi ses frères.
    ReparentEntityCommand(EditorContext &context, Engine::UUID entityID, Engine::UUID newParent,
                          Engine::UUID insertBefore);

    void Redo() override;
    void Undo() override;
    std::string GetName() const override;

private:
    EditorContext &m_Context;
    Engine::UUID m_EntityID;
    std::string m_EntityName;

    Engine::UUID m_NewParent;
    Engine::UUID m_InsertBefore;

    // État d'avant, capturé à la construction : de quoi revenir exactement en place.
    Engine::UUID m_OldParent;
    size_t m_OldIndex = 0;
};

class DeleteEntityCommand : public EditorCommand
{
public:
    DeleteEntityCommand(EditorContext &context, Engine::Entity entity);

    void Redo() override;
    void Undo() override;
    std::string GetName() const override;

private:
    EditorContext &m_Context;
    Engine::UUID m_EntityID;
    std::string m_EntityName;

    // Supprimer un parent supprime ses enfants : la sauvegarde porte donc sur toute
    // la branche, qui doit revenir d'un bloc.
    struct BackedUpEntity
    {
        Engine::UUID ID;
        std::string Name;

        // Position dans la hiérarchie au moment de la suppression : l'annulation doit
        // remettre l'entité là où elle était, pas l'ajouter en bas de la liste.
        size_t OrderIndex = 0;

        Engine::Entity Copy;
    };

    // Le parent de la racine de la branche vit en dehors de celle-ci : il n'est donc
    // pas reproduit dans la scène de sauvegarde et doit être retenu à part.
    Engine::UUID m_RootParent{0};

    // Copie hors scène des components des entités supprimées, seule chose qui permette
    // de les faire revenir intactes.
    std::shared_ptr<Engine::Scene> m_Backup;
    std::vector<BackedUpEntity> m_BackedUp;
};

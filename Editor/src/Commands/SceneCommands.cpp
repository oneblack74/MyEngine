#include "Commands/SceneCommands.h"

namespace
{
    // Scène détachée servant de presse-papiers / sauvegarde : elle n'est ni rendue ni
    // simulée, elle ne fait que garder une entité en vie hors de la scène d'édition.
    std::shared_ptr<Engine::Scene> MakeDetachedCopyOf(Engine::Entity entity, Engine::Entity &outCopy)
    {
        auto holder = std::make_shared<Engine::Scene>();
        outCopy = holder->CreateEntity(entity.GetComponent<Engine::TagComponent>().Tag);
        Engine::Scene::CopyComponents(entity, outCopy);
        return holder;
    }
}

// --- Renommage --------------------------------------------------------------

RenameEntityCommand::RenameEntityCommand(EditorContext &context, Engine::UUID entityID,
                                         std::string before, std::string after)
    : m_Context(context), m_EntityID(entityID), m_Before(std::move(before)), m_After(std::move(after)) {}

void RenameEntityCommand::Redo() { Apply(m_After); }
void RenameEntityCommand::Undo() { Apply(m_Before); }

std::string RenameEntityCommand::GetName() const
{
    return "rename " + m_Before;
}

void RenameEntityCommand::Apply(const std::string &name)
{
    Engine::Entity entity = m_Context.GetEditorScene().FindEntityByUUID(m_EntityID);
    if (!entity)
        return;

    entity.GetComponent<Engine::TagComponent>().Tag = name;
    m_Context.SelectEntity(entity);
}

// --- Création ---------------------------------------------------------------

CreateEntityCommand::CreateEntityCommand(EditorContext &context, std::string name)
    : m_Context(context), m_EntityName(std::move(name)) {}

void CreateEntityCommand::Redo()
{
    Engine::Entity entity = m_Context.GetEditorScene().CreateEntityWithUUID(m_EntityID, m_EntityName);
    if (m_SourceEntity)
        Engine::Scene::CopyComponents(m_SourceEntity, entity);

    m_Context.SelectEntity(entity);
}

void CreateEntityCommand::Undo()
{
    Engine::Entity entity = m_Context.GetEditorScene().FindEntityByUUID(m_EntityID);
    if (!entity)
        return;

    m_Context.GetEditorScene().DestroyEntity(entity);
    m_Context.SelectEntity({});
}

std::string CreateEntityCommand::GetName() const
{
    return "create " + m_EntityName;
}

CreateEntityFromCommand::CreateEntityFromCommand(EditorContext &context, Engine::Entity source,
                                                 std::string actionName)
    : CreateEntityCommand(context, source.GetComponent<Engine::TagComponent>().Tag),
      m_ActionName(std::move(actionName))
{
    // Le modèle est copié hors scène dès la construction : l'entité d'origine peut très
    // bien avoir été supprimée quand la commande est rejouée.
    m_Source = MakeDetachedCopyOf(source, m_SourceEntity);
}

std::string CreateEntityFromCommand::GetName() const
{
    return m_ActionName + " " + m_EntityName;
}

// --- Suppression ------------------------------------------------------------

DeleteEntityCommand::DeleteEntityCommand(EditorContext &context, Engine::Entity entity)
    : m_Context(context), m_EntityID(entity.GetUUID()),
      m_EntityName(entity.GetComponent<Engine::TagComponent>().Tag),
      m_OrderIndex(context.GetEditorScene().GetEntityOrderIndex(entity.GetUUID()))
{
    m_Backup = MakeDetachedCopyOf(entity, m_BackupEntity);
}

void DeleteEntityCommand::Redo()
{
    Engine::Entity entity = m_Context.GetEditorScene().FindEntityByUUID(m_EntityID);
    if (!entity)
        return;

    m_Context.GetEditorScene().DestroyEntity(entity);
    m_Context.SelectEntity({});
}

void DeleteEntityCommand::Undo()
{
    Engine::Entity restored = m_Context.GetEditorScene().CreateEntityWithUUID(m_EntityID, m_EntityName);
    Engine::Scene::CopyComponents(m_BackupEntity, restored);
    m_Context.GetEditorScene().SetEntityOrderIndex(m_EntityID, m_OrderIndex);
    m_Context.SelectEntity(restored);
}

std::string DeleteEntityCommand::GetName() const
{
    return "delete " + m_EntityName;
}

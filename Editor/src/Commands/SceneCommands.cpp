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

namespace
{
    // L'entité puis tous ses descendants, parents avant enfants.
    void CollectBranch(Engine::Scene &scene, Engine::Entity entity, std::vector<Engine::Entity> &out)
    {
        out.push_back(entity);
        for (Engine::Entity child : scene.GetChildren(entity))
            CollectBranch(scene, child, out);
    }
}

DeleteEntityCommand::DeleteEntityCommand(EditorContext &context, Engine::Entity entity)
    : m_Context(context), m_EntityID(entity.GetUUID()),
      m_EntityName(entity.GetComponent<Engine::TagComponent>().Tag)
{
    Engine::Scene &scene = context.GetEditorScene();

    Engine::Entity parent = scene.GetParent(entity);
    m_RootParent = parent ? parent.GetUUID() : Engine::UUID(0);

    std::vector<Engine::Entity> branch;
    CollectBranch(scene, entity, branch);

    // Deux passes, ici comme dans Scene::Copy : les entités existent toutes avant qu'on
    // copie le moindre component, sinon les liens de parenté internes à la branche
    // seraient vus comme pointant vers des entités absentes et seraient perdus.
    m_Backup = std::make_shared<Engine::Scene>();
    for (Engine::Entity source : branch)
    {
        BackedUpEntity backup;
        backup.ID = source.GetUUID();
        backup.Name = source.GetComponent<Engine::TagComponent>().Tag;
        backup.OrderIndex = scene.GetEntityOrderIndex(backup.ID);
        backup.Copy = m_Backup->CreateEntityWithUUID(backup.ID, backup.Name);
        m_BackedUp.push_back(backup);
    }

    for (size_t i = 0; i < branch.size(); ++i)
        Engine::Scene::CopyComponents(branch[i], m_BackedUp[i].Copy);
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
    Engine::Scene &scene = m_Context.GetEditorScene();

    // Même découpage en deux passes qu'à la sauvegarde, pour la même raison.
    for (const BackedUpEntity &backup : m_BackedUp)
        scene.CreateEntityWithUUID(backup.ID, backup.Name);

    for (const BackedUpEntity &backup : m_BackedUp)
        Engine::Scene::CopyComponents(backup.Copy, scene.FindEntityByUUID(backup.ID));

    // Le parent de la racine est extérieur à la branche : il n'était pas dans la
    // sauvegarde, donc CopyComponents ne l'a pas rétabli.
    scene.FindEntityByUUID(m_EntityID).GetComponent<Engine::ParentComponent>().Parent = m_RootParent;

    for (const BackedUpEntity &backup : m_BackedUp)
        scene.SetEntityOrderIndex(backup.ID, backup.OrderIndex);

    m_Context.SelectEntity(scene.FindEntityByUUID(m_EntityID));
}

std::string DeleteEntityCommand::GetName() const
{
    return "delete " + m_EntityName;
}

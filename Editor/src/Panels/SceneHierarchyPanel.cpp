#include "Panels/SceneHierarchyPanel.h"
#include <Scene/Components.h>
#include <Scene/SceneSerializer.h>
#include <filesystem>
#include <imgui.h>
#include <imgui_internal.h> // BeginDragDropTargetCustom : API "interne", mais c'est le seul moyen d'avoir plusieurs zones de dépôt sur une même ligne

namespace
{
    // Hauteur de la bande de dépôt "insérer avant", en proportion de la ligne. Le reste
    // de la ligne rattache l'entité déposée — c'est l'interaction habituelle des arbres
    // d'éditeurs (Unity, Godot).
    constexpr float k_InsertBandRatio = 0.3f;

    Engine::UUID PayloadEntity(const ImGuiPayload *payload)
    {
        uint64_t id = 0;
        memcpy(&id, payload->Data, sizeof(id));
        return Engine::UUID(id);
    }
}

bool SceneHierarchyPanel::AcceptSceneDrop(Engine::UUID target)
{
    const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(k_AssetPayloadType);
    if (!payload)
        return false;

    // La charge utile est un chemin relatif, terminé par un zéro (ContentBrowserPanel).
    const std::string path((const char *)payload->Data);

    // Seules les scènes s'instancient : déposer une texture ici n'a pas de sens.
    if (std::filesystem::path(path).extension() != Engine::k_SceneExtension)
        return true;

    m_PendingSceneInstance = {path, target};
    m_HasPendingSceneInstance = true;
    return true;
}

void SceneHierarchyPanel::OnImGuiRender()
{
    ImGui::Begin("Scene Hierarchy");

    if (m_Context)
    {
        // La scène coiffe ses entités, comme dans Unity. SpanAvailWidth pour que la
        // ligne entière réagisse, et pas seulement le texte.
        const ImGuiTreeNodeFlags sceneFlags = ImGuiTreeNodeFlags_DefaultOpen |
                                              ImGuiTreeNodeFlags_SpanAvailWidth |
                                              ImGuiTreeNodeFlags_OpenOnArrow;
        const bool open = ImGui::TreeNodeEx("##scene", sceneFlags, "%s", m_SceneName.c_str());

        // Déposer sur la scène elle-même détache l'entité de son parent.
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(k_EntityPayloadType))
            {
                m_PendingDrop = {PayloadEntity(payload), Engine::UUID(0), false};
                m_HasPendingDrop = true;
            }
            // Une scène déposée sur la scène elle-même s'instancie sous sa racine.
            AcceptSceneDrop(Engine::UUID(0));
            ImGui::EndDragDropTarget();
        }

        if (open)
        {
            // Seules les entités racine sont parcourues ici : chaque nœud dessine ses
            // propres enfants.
            for (Engine::Entity entity : m_Context->GetRootEntities())
                DrawEntityNode(entity);

            ImGui::TreePop();
        }
    }

    ImGui::End();
}

void SceneHierarchyPanel::DrawEntityNode(Engine::Entity entity)
{
    const std::string &tag = entity.GetComponent<Engine::TagComponent>().Tag;
    const std::vector<Engine::Entity> children = m_Context->GetChildren(entity);

    ImGuiTreeNodeFlags flags = (m_SelectionContext == entity ? ImGuiTreeNodeFlags_Selected : 0) |
                               ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth |
                               ImGuiTreeNodeFlags_DefaultOpen;
    if (children.empty())
        flags |= ImGuiTreeNodeFlags_Leaf;

    // L'ID vient de l'UUID et non du handle EnTT : le handle est recyclé et change à
    // chaque copie de scène, ce qui ferait replier les nœuds au moindre Play.
    const Engine::UUID uuid = entity.GetUUID();
    const bool open = ImGui::TreeNodeEx((void *)(uint64_t)uuid, flags, "%s", tag.c_str());

    // IsItemToggledOpen : cliquer la flèche ne doit pas changer la sélection.
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
        m_SelectionContext = entity;

    if (ImGui::BeginDragDropSource())
    {
        const uint64_t id = uuid;
        ImGui::SetDragDropPayload(k_EntityPayloadType, &id, sizeof(id));
        ImGui::TextUnformatted(tag.c_str());
        ImGui::EndDragDropSource();
    }

    // Deux zones de dépôt sur la même ligne : le haut insère avant (réordonner), le
    // reste rattache (devenir enfant). D'où les rectangles explicites plutôt qu'un
    // simple BeginDragDropTarget, qui prendrait toute la ligne.
    const ImVec2 itemMin = ImGui::GetItemRectMin();
    const ImVec2 itemMax = ImGui::GetItemRectMax();
    const float bandHeight = (itemMax.y - itemMin.y) * k_InsertBandRatio;

    const ImRect insertRect(itemMin, ImVec2(itemMax.x, itemMin.y + bandHeight));
    if (ImGui::BeginDragDropTargetCustom(insertRect, ImGui::GetID("##insert")))
    {
        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(k_EntityPayloadType))
        {
            m_PendingDrop = {PayloadEntity(payload), uuid, true};
            m_HasPendingDrop = true;
        }
        ImGui::EndDragDropTarget();
    }

    const ImRect parentRect(ImVec2(itemMin.x, itemMin.y + bandHeight), itemMax);
    if (ImGui::BeginDragDropTargetCustom(parentRect, ImGui::GetID("##parent")))
    {
        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(k_EntityPayloadType))
        {
            m_PendingDrop = {PayloadEntity(payload), uuid, false};
            m_HasPendingDrop = true;
        }
        // Déposer une scène sur une entité l'instancie comme son enfant.
        AcceptSceneDrop(uuid);
        ImGui::EndDragDropTarget();
    }

    if (open)
    {
        for (Engine::Entity child : children)
            DrawEntityNode(child);

        ImGui::TreePop();
    }
}

bool SceneHierarchyPanel::TakePendingSceneInstance(SceneInstanceDrop &out)
{
    if (!m_HasPendingSceneInstance)
        return false;

    out = m_PendingSceneInstance;
    m_HasPendingSceneInstance = false;
    return true;
}

bool SceneHierarchyPanel::TakePendingDrop(HierarchyDrop &out)
{
    if (!m_HasPendingDrop)
        return false;

    out = m_PendingDrop;
    m_HasPendingDrop = false;
    return true;
}

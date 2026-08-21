#include "Panels/SceneHierarchyPanel.h"
#include <Scene/Components.h>
#include <imgui.h>

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
        if (ImGui::TreeNodeEx("##scene", sceneFlags, "%s", m_SceneName.c_str()))
        {
            // Parcours de l'ordre de la scène et non d'une vue EnTT : c'est ce qui
            // garantit qu'une entité ne change pas de place quand on en supprime une
            // autre (le registre déplace alors sa dernière entité dans le trou laissé).
            for (Engine::UUID uuid : m_Context->GetEntityOrder())
            {
                Engine::Entity entity = m_Context->FindEntityByUUID(uuid);
                if (entity)
                    DrawEntityNode(entity);
            }

            ImGui::TreePop();
        }
    }

    ImGui::End();
}

void SceneHierarchyPanel::DrawEntityNode(Engine::Entity entity)
{
    auto &tag = entity.GetComponent<Engine::TagComponent>().Tag;

    ImGuiTreeNodeFlags flags = (m_SelectionContext == entity ? ImGuiTreeNodeFlags_Selected : 0) |
                                ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    ImGui::TreeNodeEx((void *)(uint64_t)(uint32_t)entity, flags, "%s", tag.c_str());
    if (ImGui::IsItemClicked())
        m_SelectionContext = entity;
}

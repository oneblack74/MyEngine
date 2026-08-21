#include "Panels/SceneHierarchyPanel.h"
#include <Scene/Components.h>
#include <imgui.h>

void SceneHierarchyPanel::OnImGuiRender()
{
    ImGui::Begin("Scene Hierarchy");

    if (m_Context)
    {
        auto view = m_Context->GetAllEntitiesWith<Engine::IDComponent>();
        for (auto entityHandle : view)
        {
            Engine::Entity entity{entityHandle, m_Context.get()};
            DrawEntityNode(entity);
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

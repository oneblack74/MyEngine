#include "Panels/InspectorPanel.h"
#include <Scene/Components.h>
#include <imgui.h>
#include <cstring>

void InspectorPanel::OnImGuiRender(Engine::Entity selectedEntity)
{
    ImGui::Begin("Inspecteur");

    if (selectedEntity)
        DrawComponents(selectedEntity);

    ImGui::End();
}

void InspectorPanel::DrawComponents(Engine::Entity entity)
{
    if (entity.HasComponent<Engine::TagComponent>())
    {
        auto &tag = entity.GetComponent<Engine::TagComponent>().Tag;

        char buffer[256];
        memset(buffer, 0, sizeof(buffer));
        strncpy(buffer, tag.c_str(), sizeof(buffer) - 1);
        if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
            tag = std::string(buffer);
    }

    ImGui::Separator();

    if (entity.HasComponent<Engine::TransformComponent>())
    {
        if (ImGui::TreeNodeEx("Transform", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto &transform = entity.GetComponent<Engine::TransformComponent>();
            ImGui::DragFloat3("Position", &transform.Position.x, 0.05f);
            ImGui::DragFloat("Rotation", &transform.Rotation, 0.5f);
            ImGui::DragFloat3("Scale", &transform.Scale.x, 0.05f);
            ImGui::TreePop();
        }
    }

    if (entity.HasComponent<Engine::SpriteRendererComponent>())
    {
        if (ImGui::TreeNodeEx("Sprite Renderer", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto &sprite = entity.GetComponent<Engine::SpriteRendererComponent>();
            ImGui::ColorEdit4("Couleur", &sprite.Color.x);
            ImGui::DragFloat("Tiling", &sprite.TilingFactor, 0.05f, 0.0f, 100.0f);
            ImGui::TreePop();
        }
    }
}

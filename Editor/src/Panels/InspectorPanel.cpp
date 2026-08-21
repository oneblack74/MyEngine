#include "Panels/InspectorPanel.h"
#include <Scene/Components.h>
#include <imgui.h>
#include <cstring>

namespace
{
    // Les champs bornés (min/max) doivent aussi être bornés quand la valeur est *tapée*
    // au clavier : sans ce flag, ImGui laisse la saisie manuelle sortir des limites
    // (un rayon négatif ferait planter Box2D, par exemple).
    constexpr ImGuiSliderFlags k_ClampedDrag = ImGuiSliderFlags_AlwaysClamp;
}

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
            ImGui::DragFloat("Tiling", &sprite.TilingFactor, 0.05f, 0.0f, 100.0f, "%.3f", k_ClampedDrag);
            ImGui::TreePop();
        }
    }

    if (entity.HasComponent<Engine::RigidBodyComponent>())
    {
        if (ImGui::TreeNodeEx("Rigid Body", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto &rb = entity.GetComponent<Engine::RigidBodyComponent>();

            const char *typeLabels[] = {"Static", "Dynamic", "Kinematic"};
            int currentType = (int)rb.Type;
            if (ImGui::Combo("Type", &currentType, typeLabels, 3))
                rb.Type = (Engine::RigidBodyComponent::BodyType)currentType;

            ImGui::Checkbox("Rotation fixe", &rb.FixedRotation);
            ImGui::TreePop();
        }
    }

    if (entity.HasComponent<Engine::BoxColliderComponent>())
    {
        if (ImGui::TreeNodeEx("Box Collider", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto &bc = entity.GetComponent<Engine::BoxColliderComponent>();
            ImGui::DragFloat2("Offset", &bc.Offset.x, 0.05f);
            ImGui::DragFloat2("Taille (demi)", &bc.Size.x, 0.05f, 0.01f, 100.0f, "%.3f", k_ClampedDrag);
            ImGui::DragFloat("Densité", &bc.Density, 0.05f, 0.0f, 100.0f, "%.3f", k_ClampedDrag);
            ImGui::DragFloat("Friction", &bc.Friction, 0.01f, 0.0f, 1.0f, "%.3f", k_ClampedDrag);
            ImGui::DragFloat("Restitution", &bc.Restitution, 0.01f, 0.0f, 1.0f, "%.3f", k_ClampedDrag);
            ImGui::TreePop();
        }
    }

    if (entity.HasComponent<Engine::CircleColliderComponent>())
    {
        if (ImGui::TreeNodeEx("Circle Collider", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto &cc = entity.GetComponent<Engine::CircleColliderComponent>();
            ImGui::DragFloat2("Offset", &cc.Offset.x, 0.05f);
            ImGui::DragFloat("Rayon", &cc.Radius, 0.05f, 0.01f, 100.0f, "%.3f", k_ClampedDrag);
            ImGui::DragFloat("Densité", &cc.Density, 0.05f, 0.0f, 100.0f, "%.3f", k_ClampedDrag);
            ImGui::DragFloat("Friction", &cc.Friction, 0.01f, 0.0f, 1.0f, "%.3f", k_ClampedDrag);
            ImGui::DragFloat("Restitution", &cc.Restitution, 0.01f, 0.0f, 1.0f, "%.3f", k_ClampedDrag);
            ImGui::TreePop();
        }
    }

    if (entity.HasComponent<Engine::CameraComponent>())
    {
        if (ImGui::TreeNodeEx("Camera", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto &cc = entity.GetComponent<Engine::CameraComponent>();
            ImGui::Checkbox("Principale (Primary)", &cc.Primary);
            ImGui::DragFloat("Taille orthographique", &cc.OrthographicSize, 0.05f, 0.05f, 100.0f, "%.3f", k_ClampedDrag);
            ImGui::TreePop();
        }
    }
}

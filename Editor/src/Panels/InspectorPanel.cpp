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

    // En-tête d'une section de component : le TreeNode habituel plus un bouton
    // "Réinit." aligné à droite de la même ligne. Renvoie true si la section est
    // ouverte ; resetRequested passe à true la frame où le bouton est cliqué.
    // À refermer avec EndComponentSection().
    bool BeginComponentSection(const char *label, bool &resetRequested)
    {
        ImGui::PushID(label);

        const ImGuiStyle &style = ImGui::GetStyle();
        const char *resetLabel = "Réinit.";
        const float buttonWidth = ImGui::CalcTextSize(resetLabel).x + style.FramePadding.x * 2.0f;
        const float lineStartX = ImGui::GetCursorPosX();
        const float availWidth = ImGui::GetContentRegionAvail().x;

        // AllowOverlap : sans ça le TreeNode capterait le clic destiné au bouton posé
        // par-dessus sa ligne.
        const bool open = ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowOverlap);

        ImGui::SameLine(lineStartX + availWidth - buttonWidth);
        resetRequested = ImGui::SmallButton(resetLabel);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Réinitialiser %s aux valeurs par défaut", label);

        return open;
    }

    void EndComponentSection(bool open)
    {
        if (open)
            ImGui::TreePop();
        ImGui::PopID();
    }
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
        bool reset = false;
        const bool open = BeginComponentSection("Transform", reset);

        auto &transform = entity.GetComponent<Engine::TransformComponent>();
        if (reset)
            transform = Engine::TransformComponent{};

        if (open)
        {
            ImGui::DragFloat3("Position", &transform.Position.x, 0.05f);
            ImGui::DragFloat("Rotation", &transform.Rotation, 0.5f);
            ImGui::DragFloat3("Scale", &transform.Scale.x, 0.05f);
        }

        EndComponentSection(open);
    }

    if (entity.HasComponent<Engine::SpriteRendererComponent>())
    {
        bool reset = false;
        const bool open = BeginComponentSection("Sprite Renderer", reset);

        auto &sprite = entity.GetComponent<Engine::SpriteRendererComponent>();
        if (reset)
        {
            const Engine::SpriteRendererComponent defaults;
            sprite.Color = defaults.Color;
            sprite.TilingFactor = defaults.TilingFactor;
            // Texture volontairement conservée : elle n'est pas éditable depuis
            // l'Inspecteur (pas d'asset system avant la Phase 7), la remettre à null
            // la perdrait sans aucun moyen de la retrouver.
        }

        if (open)
        {
            ImGui::ColorEdit4("Couleur", &sprite.Color.x);
            ImGui::DragFloat("Tiling", &sprite.TilingFactor, 0.05f, 0.0f, 100.0f, "%.3f", k_ClampedDrag);
        }

        EndComponentSection(open);
    }

    if (entity.HasComponent<Engine::RigidBodyComponent>())
    {
        bool reset = false;
        const bool open = BeginComponentSection("Rigid Body", reset);

        auto &rb = entity.GetComponent<Engine::RigidBodyComponent>();
        if (reset)
        {
            // Champ par champ, et pas rb = {} : RuntimeBody est le handle Box2D du corps
            // en cours de simulation, l'écraser perdrait le corps pendant le Play.
            const Engine::RigidBodyComponent defaults;
            rb.Type = defaults.Type;
            rb.FixedRotation = defaults.FixedRotation;
        }

        if (open)
        {
            const char *typeLabels[] = {"Static", "Dynamic", "Kinematic"};
            int currentType = (int)rb.Type;
            if (ImGui::Combo("Type", &currentType, typeLabels, 3))
                rb.Type = (Engine::RigidBodyComponent::BodyType)currentType;

            ImGui::Checkbox("Rotation fixe", &rb.FixedRotation);
        }

        EndComponentSection(open);
    }

    if (entity.HasComponent<Engine::BoxColliderComponent>())
    {
        bool reset = false;
        const bool open = BeginComponentSection("Box Collider", reset);

        auto &bc = entity.GetComponent<Engine::BoxColliderComponent>();
        if (reset)
        {
            // RuntimeShape préservé, même raison que RigidBodyComponent::RuntimeBody.
            const Engine::BoxColliderComponent defaults;
            bc.Offset = defaults.Offset;
            bc.Size = defaults.Size;
            bc.Density = defaults.Density;
            bc.Friction = defaults.Friction;
            bc.Restitution = defaults.Restitution;
        }

        if (open)
        {
            ImGui::DragFloat2("Offset", &bc.Offset.x, 0.05f);
            ImGui::DragFloat2("Taille (demi)", &bc.Size.x, 0.05f, 0.01f, 100.0f, "%.3f", k_ClampedDrag);
            ImGui::DragFloat("Densité", &bc.Density, 0.05f, 0.0f, 100.0f, "%.3f", k_ClampedDrag);
            ImGui::DragFloat("Friction", &bc.Friction, 0.01f, 0.0f, 1.0f, "%.3f", k_ClampedDrag);
            ImGui::DragFloat("Restitution", &bc.Restitution, 0.01f, 0.0f, 1.0f, "%.3f", k_ClampedDrag);
        }

        EndComponentSection(open);
    }

    if (entity.HasComponent<Engine::CircleColliderComponent>())
    {
        bool reset = false;
        const bool open = BeginComponentSection("Circle Collider", reset);

        auto &cc = entity.GetComponent<Engine::CircleColliderComponent>();
        if (reset)
        {
            const Engine::CircleColliderComponent defaults;
            cc.Offset = defaults.Offset;
            cc.Radius = defaults.Radius;
            cc.Density = defaults.Density;
            cc.Friction = defaults.Friction;
            cc.Restitution = defaults.Restitution;
        }

        if (open)
        {
            ImGui::DragFloat2("Offset", &cc.Offset.x, 0.05f);
            ImGui::DragFloat("Rayon", &cc.Radius, 0.05f, 0.01f, 100.0f, "%.3f", k_ClampedDrag);
            ImGui::DragFloat("Densité", &cc.Density, 0.05f, 0.0f, 100.0f, "%.3f", k_ClampedDrag);
            ImGui::DragFloat("Friction", &cc.Friction, 0.01f, 0.0f, 1.0f, "%.3f", k_ClampedDrag);
            ImGui::DragFloat("Restitution", &cc.Restitution, 0.01f, 0.0f, 1.0f, "%.3f", k_ClampedDrag);
        }

        EndComponentSection(open);
    }

    if (entity.HasComponent<Engine::CameraComponent>())
    {
        bool reset = false;
        const bool open = BeginComponentSection("Camera", reset);

        auto &cc = entity.GetComponent<Engine::CameraComponent>();
        if (reset)
        {
            // Camera (l'OrthographicCamera embarquée) est reconstruite à chaque rendu
            // par GamePanel, rien à réinitialiser dessus.
            const Engine::CameraComponent defaults;
            cc.Primary = defaults.Primary;
            cc.OrthographicSize = defaults.OrthographicSize;
        }

        if (open)
        {
            ImGui::Checkbox("Principale (Primary)", &cc.Primary);
            ImGui::DragFloat("Taille orthographique", &cc.OrthographicSize, 0.05f, 0.05f, 100.0f, "%.3f", k_ClampedDrag);
        }

        EndComponentSection(open);
    }
}

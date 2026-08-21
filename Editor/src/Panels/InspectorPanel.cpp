#include "Panels/InspectorPanel.h"
#include "Panels/ContentBrowserPanel.h"
#include <Assets/AssetManager.h>
#include <filesystem>
#include <Scene/Components.h>
#include <Scene/SceneSerializer.h>
#include <functional>
#include <imgui.h>
#include <vector>
#include <cstring>
#include <cstdio>

namespace
{
    // Les champs bornés (min/max) doivent aussi être bornés quand la valeur est *tapée*
    // au clavier : sans ce flag, ImGui laisse la saisie manuelle sortir des limites
    // (un rayon négatif ferait planter Box2D, par exemple).
    constexpr ImGuiSliderFlags k_ClampedDrag = ImGuiSliderFlags_AlwaysClamp;

    // Reçoit un asset glissé depuis le Content Browser sur le widget qui vient d'être
    // dessiné. Le type attendu est vérifié : déposer un son sur un champ de texture ne
    // doit rien faire, plutôt que d'installer une référence invalide.
    bool AcceptAssetDrop(Engine::AssetType expected, Engine::AssetHandle &outHandle)
    {
        if (!ImGui::BeginDragDropTarget())
            return false;

        bool accepted = false;
        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(k_AssetPayloadType))
        {
            const std::string path((const char *)payload->Data);
            if (Engine::AssetTypeFromExtension(std::filesystem::path(path).extension().string()) == expected)
            {
                outHandle = Engine::AssetManager::Import(path);
                accepted = true;
            }
        }

        ImGui::EndDragDropTarget();
        return accepted;
    }

    void EndComponentSection(bool open)
    {
        if (open)
            ImGui::TreePop();
    }
}

void InspectorPanel::OnImGuiRender(Engine::Entity selectedEntity, CommandHistory *history)
{
    m_History = history;

    ImGui::Begin("Inspector");

    // Recalculées à chaque frame pour la seule entité affichée : le coût est celui de
    // sérialiser une entité, et l'affichage suit toute modification sans être prévenu.
    m_Overrides = m_InstanceSync != nullptr ? m_InstanceSync->OverriddenPropertiesOf(selectedEntity)
                                            : std::set<std::string>{};

    if (selectedEntity)
        DrawComponents(selectedEntity);

    ImGui::End();

    // Après le dessin : l'action peut retirer ou réécrire un component, ce qui
    // invaliderait les références tenues par les sections en cours.
    ApplyPendingAction();
}

bool InspectorPanel::BeginComponentSection(Engine::Entity entity, const char *label,
                                           const char *componentName, bool &resetRequested)
{
    const ImGuiStyle &style = ImGui::GetStyle();
    const float buttonWidth = ImGui::CalcTextSize("Reset").x + style.FramePadding.x * 2.0f;
    const float lineStartX = ImGui::GetCursorPosX();
    const float availWidth = ImGui::GetContentRegionAvail().x;

    // AllowOverlap : sans ça le TreeNode capterait le clic destiné au bouton posé
    // par-dessus sa ligne.
    const bool open = ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowOverlap);

    // Le menu contextuel s'accroche au TreeNode, donc avant que le bouton "Reset" ne
    // devienne le dernier item.
    DrawComponentContextMenu(entity, componentName);

    // Le TreeNode ouvert empile déjà son propre ID, ce qui suffit à distinguer les
    // boutons entre sections ; replié il n'empile rien, d'où le suffixe ## explicite.
    char buttonId[64];
    snprintf(buttonId, sizeof(buttonId), "Reset##%s", label);

    ImGui::SameLine(lineStartX + availWidth - buttonWidth);
    resetRequested = ImGui::SmallButton(buttonId);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Reset %s to its default values", label);

    return open;
}

void InspectorPanel::MarkOverride(const char *componentName, const char *field)
{
    if (m_Overrides.find(std::string(componentName) + "/" + field) == m_Overrides.end())
        return;

    // Un liseré vertical au bord gauche du panel, en face de la ligne du widget.
    const float left = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMin().x;
    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(left - 6.0f, min.y), ImVec2(left - 3.0f, max.y),
                                              ImGui::GetColorU32(ImVec4(0.35f, 0.65f, 1.0f, 1.0f)));
}

void InspectorPanel::DrawComponentContextMenu(Engine::Entity entity, const char *componentName)
{
    if (!ImGui::BeginPopupContextItem(componentName))
        return;

    const bool editable = m_History != nullptr && m_Context != nullptr;

    if (ImGui::MenuItem("Remove Component", nullptr, false, editable))
    {
        m_PendingAction = {PendingAction::Kind::RemoveComponent, entity, componentName, {}};
        m_HasPendingAction = true;
    }

    // Les surcharges de ce component, révocables une par une ou toutes ensemble.
    std::vector<std::string> overriddenFields;
    const std::string prefix = std::string(componentName) + "/";
    for (const std::string &key : m_Overrides)
    {
        if (key.rfind(prefix, 0) == 0)
            overriddenFields.push_back(key.substr(prefix.size()));
    }

    if (m_InstanceSync != nullptr && !overriddenFields.empty())
    {
        ImGui::Separator();

        if (ImGui::MenuItem("Revert Component to Source"))
        {
            m_PendingAction = {PendingAction::Kind::RevertComponent, entity, componentName, {}};
            m_HasPendingAction = true;
        }

        if (ImGui::BeginMenu("Revert Property"))
        {
            for (const std::string &field : overriddenFields)
            {
                if (ImGui::MenuItem(field.c_str()))
                {
                    m_PendingAction = {PendingAction::Kind::RevertProperty, entity, componentName, field};
                    m_HasPendingAction = true;
                }
            }
            ImGui::EndMenu();
        }
    }

    ImGui::EndPopup();
}

void InspectorPanel::ApplyPendingAction()
{
    if (!m_HasPendingAction)
        return;

    const PendingAction action = m_PendingAction;
    m_HasPendingAction = false;

    Engine::Entity entity = action.Target;
    if (!entity)
        return;

    if (action.Type == PendingAction::Kind::RemoveComponent)
    {
        if (m_History != nullptr && m_Context != nullptr)
        {
            m_History->Execute(std::make_unique<RemoveComponentCommand>(*m_Context, entity.GetUUID(),
                                                                        action.Component));
        }
        return;
    }

    if (m_InstanceSync == nullptr)
        return;

    // La révocation écrit directement dans les components : la commande n'est empilée
    // qu'ensuite, pour rendre le retour en arrière possible sans réappliquer.
    const nlohmann::json before = Engine::SceneSerializer::EntityToJson(entity);
    std::string name = action.Component;

    if (action.Type == PendingAction::Kind::RevertComponent)
    {
        m_InstanceSync->RevertComponent(entity, action.Component);
    }
    else
    {
        m_InstanceSync->RevertProperty(entity, action.Component, action.Field);
        name = action.Field;
    }

    if (m_History != nullptr && m_Context != nullptr)
    {
        m_History->PushAlreadyApplied(std::make_unique<EntityJsonEditCommand>(
            *m_Context, entity.GetUUID(), before, Engine::SceneSerializer::EntityToJson(entity),
            "revert " + name));
    }
}

void InspectorPanel::DrawComponents(Engine::Entity entity)
{
    if (entity.HasComponent<Engine::TagComponent>())
    {
        auto &tag = entity.GetComponent<Engine::TagComponent>().Tag;

        char buffer[256];
        memset(buffer, 0, sizeof(buffer));
        strncpy(buffer, tag.c_str(), sizeof(buffer) - 1);

        if (ImGui::IsItemActivated())
            m_NameBeforeEdit = tag;
        if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
            tag = std::string(buffer);
        MarkOverride("TagComponent", "Tag");
        // Une seule commande pour toute la saisie, empilée quand le champ perd le focus.
        if (ImGui::IsItemDeactivatedAfterEdit() && m_History != nullptr && m_Context != nullptr &&
            m_NameBeforeEdit != tag)
        {
            m_History->PushAlreadyApplied(std::make_unique<RenameEntityCommand>(
                *m_Context, entity.GetUUID(), m_NameBeforeEdit, tag));
        }
    }

    ImGui::Separator();

    if (entity.HasComponent<Engine::TransformComponent>())
    {
        bool reset = false;
        const bool open = BeginComponentSection(entity, "Transform", "TransformComponent", reset);

        auto &transform = entity.GetComponent<Engine::TransformComponent>();
        if (reset)
        {
            const Engine::TransformComponent before = transform;
            transform = Engine::TransformComponent{};
            RecordEdit(entity, before, "reset Transform of");
        }

        if (open)
        {
            ImGui::DragFloat3("Position", &transform.Position.x, 0.05f);
            MarkOverride("TransformComponent", "Position");
            TrackEdit<Engine::TransformComponent>(entity, "move");
            ImGui::DragFloat("Rotation", &transform.Rotation, 0.5f);
            MarkOverride("TransformComponent", "Rotation");
            TrackEdit<Engine::TransformComponent>(entity, "rotate");
            ImGui::DragFloat3("Scale", &transform.Scale.x, 0.05f);
            MarkOverride("TransformComponent", "Scale");
            TrackEdit<Engine::TransformComponent>(entity, "scale");
        }

        EndComponentSection(open);
    }

    if (entity.HasComponent<Engine::SpriteRendererComponent>())
    {
        bool reset = false;
        const bool open = BeginComponentSection(entity, "Sprite Renderer", "SpriteRendererComponent", reset);

        auto &sprite = entity.GetComponent<Engine::SpriteRendererComponent>();
        if (reset)
        {
            const Engine::SpriteRendererComponent before = sprite;
            const Engine::SpriteRendererComponent defaults;
            sprite.Color = defaults.Color;
            sprite.TilingFactor = defaults.TilingFactor;
            // Texture volontairement conservée : elle n'est pas éditable depuis
            // l'Inspecteur (pas d'asset system avant la Phase 7), la remettre à null
            // la perdrait sans aucun moyen de la retrouver.
            RecordEdit(entity, before, "reset Sprite Renderer of");
        }

        if (open)
        {
            // La texture s'édite par son chemin : le Content Browser ne sait pas encore
            // glisser-déposer un asset, l'import se fait donc à la saisie.
            // Copie prise avant le widget : un dépôt est instantané, sa valeur d'avant
            // ne peut pas être capturée au moment où on constate le changement.
            const Engine::SpriteRendererComponent spriteBeforeDrop = sprite;

            char texturePath[512];
            memset(texturePath, 0, sizeof(texturePath));
            strncpy(texturePath, Engine::AssetManager::GetPath(sprite.Texture).c_str(), sizeof(texturePath) - 1);
            if (ImGui::InputText("Texture", texturePath, sizeof(texturePath)))
            {
                const std::string path(texturePath);
                sprite.Texture = path.empty() ? Engine::AssetHandle(Engine::k_InvalidAssetHandle)
                                              : Engine::AssetManager::Import(path);
            }
            MarkOverride("SpriteRendererComponent", "Texture");
            if (AcceptAssetDrop(Engine::AssetType::Texture, sprite.Texture))
                RecordEdit(entity, spriteBeforeDrop, "change texture of");
            TrackEdit<Engine::SpriteRendererComponent>(entity, "change texture of");

            ImGui::ColorEdit4("Color", &sprite.Color.x);
            MarkOverride("SpriteRendererComponent", "Color");
            TrackEdit<Engine::SpriteRendererComponent>(entity, "change color of");
            ImGui::DragFloat("Tiling", &sprite.TilingFactor, 0.05f, 0.0f, 100.0f, "%.3f", k_ClampedDrag);
            MarkOverride("SpriteRendererComponent", "TilingFactor");
            TrackEdit<Engine::SpriteRendererComponent>(entity, "change tiling of");
        }

        EndComponentSection(open);
    }

    if (entity.HasComponent<Engine::RigidBodyComponent>())
    {
        bool reset = false;
        const bool open = BeginComponentSection(entity, "Rigid Body", "RigidBodyComponent", reset);

        auto &rb = entity.GetComponent<Engine::RigidBodyComponent>();
        if (reset)
        {
            const Engine::RigidBodyComponent before = rb;
            // Champ par champ, et pas rb = {} : RuntimeBody est le handle Box2D du corps
            // en cours de simulation, l'écraser perdrait le corps pendant le Play.
            const Engine::RigidBodyComponent defaults;
            rb.Type = defaults.Type;
            rb.FixedRotation = defaults.FixedRotation;
            RecordEdit(entity, before, "reset Rigid Body of");
        }

        if (open)
        {
            const char *typeLabels[] = {"Static", "Dynamic", "Kinematic"};
            int currentType = (int)rb.Type;
            // La liste déroulante modifie sa valeur depuis un popup : contrairement à un
            // drag, l'édition est instantanée et se raccroche à son propre retour.
            const Engine::RigidBodyComponent beforeType = rb;
            if (ImGui::Combo("Type", &currentType, typeLabels, 3))
            {
                rb.Type = (Engine::RigidBodyComponent::BodyType)currentType;
                RecordEdit(entity, beforeType, "change type of");
            }
            MarkOverride("RigidBodyComponent", "Type");

            ImGui::Checkbox("Fixed Rotation", &rb.FixedRotation);
            MarkOverride("RigidBodyComponent", "FixedRotation");
            TrackEdit<Engine::RigidBodyComponent>(entity, "edit Rigid Body of");
        }

        EndComponentSection(open);
    }

    if (entity.HasComponent<Engine::BoxColliderComponent>())
    {
        bool reset = false;
        const bool open = BeginComponentSection(entity, "Box Collider", "BoxColliderComponent", reset);

        auto &bc = entity.GetComponent<Engine::BoxColliderComponent>();
        if (reset)
        {
            const Engine::BoxColliderComponent before = bc;
            // RuntimeShape préservé, même raison que RigidBodyComponent::RuntimeBody.
            const Engine::BoxColliderComponent defaults;
            bc.Offset = defaults.Offset;
            bc.Size = defaults.Size;
            bc.Density = defaults.Density;
            bc.Friction = defaults.Friction;
            bc.Restitution = defaults.Restitution;
            RecordEdit(entity, before, "reset Box Collider of");
        }

        if (open)
        {
            ImGui::DragFloat2("Offset", &bc.Offset.x, 0.05f);
            MarkOverride("BoxColliderComponent", "Offset");
            TrackEdit<Engine::BoxColliderComponent>(entity, "edit collider of");
            ImGui::DragFloat2("Size (half)", &bc.Size.x, 0.05f, 0.01f, 100.0f, "%.3f", k_ClampedDrag);
            MarkOverride("BoxColliderComponent", "Size");
            TrackEdit<Engine::BoxColliderComponent>(entity, "edit collider of");
            ImGui::DragFloat("Density", &bc.Density, 0.05f, 0.0f, 100.0f, "%.3f", k_ClampedDrag);
            MarkOverride("BoxColliderComponent", "Density");
            TrackEdit<Engine::BoxColliderComponent>(entity, "edit collider of");
            ImGui::DragFloat("Friction", &bc.Friction, 0.01f, 0.0f, 1.0f, "%.3f", k_ClampedDrag);
            MarkOverride("BoxColliderComponent", "Friction");
            TrackEdit<Engine::BoxColliderComponent>(entity, "edit collider of");
            ImGui::DragFloat("Restitution", &bc.Restitution, 0.01f, 0.0f, 1.0f, "%.3f", k_ClampedDrag);
            MarkOverride("BoxColliderComponent", "Restitution");
            TrackEdit<Engine::BoxColliderComponent>(entity, "edit collider of");
        }

        EndComponentSection(open);
    }

    if (entity.HasComponent<Engine::CircleColliderComponent>())
    {
        bool reset = false;
        const bool open = BeginComponentSection(entity, "Circle Collider", "CircleColliderComponent", reset);

        auto &cc = entity.GetComponent<Engine::CircleColliderComponent>();
        if (reset)
        {
            const Engine::CircleColliderComponent before = cc;
            const Engine::CircleColliderComponent defaults;
            cc.Offset = defaults.Offset;
            cc.Radius = defaults.Radius;
            cc.Density = defaults.Density;
            cc.Friction = defaults.Friction;
            cc.Restitution = defaults.Restitution;
            RecordEdit(entity, before, "reset Circle Collider of");
        }

        if (open)
        {
            ImGui::DragFloat2("Offset", &cc.Offset.x, 0.05f);
            MarkOverride("CircleColliderComponent", "Offset");
            TrackEdit<Engine::CircleColliderComponent>(entity, "edit collider of");
            ImGui::DragFloat("Radius", &cc.Radius, 0.05f, 0.01f, 100.0f, "%.3f", k_ClampedDrag);
            MarkOverride("CircleColliderComponent", "Radius");
            TrackEdit<Engine::CircleColliderComponent>(entity, "edit collider of");
            ImGui::DragFloat("Density", &cc.Density, 0.05f, 0.0f, 100.0f, "%.3f", k_ClampedDrag);
            MarkOverride("CircleColliderComponent", "Density");
            TrackEdit<Engine::CircleColliderComponent>(entity, "edit collider of");
            ImGui::DragFloat("Friction", &cc.Friction, 0.01f, 0.0f, 1.0f, "%.3f", k_ClampedDrag);
            MarkOverride("CircleColliderComponent", "Friction");
            TrackEdit<Engine::CircleColliderComponent>(entity, "edit collider of");
            ImGui::DragFloat("Restitution", &cc.Restitution, 0.01f, 0.0f, 1.0f, "%.3f", k_ClampedDrag);
            MarkOverride("CircleColliderComponent", "Restitution");
            TrackEdit<Engine::CircleColliderComponent>(entity, "edit collider of");
        }

        EndComponentSection(open);
    }

    if (entity.HasComponent<Engine::AudioComponent>())
    {
        bool reset = false;
        const bool open = BeginComponentSection(entity, "Audio", "AudioComponent", reset);

        auto &audio = entity.GetComponent<Engine::AudioComponent>();
        if (reset)
        {
            const Engine::AudioComponent before = audio;
            const Engine::AudioComponent defaults;
            audio.Sound = defaults.Sound;
            audio.Volume = defaults.Volume;
            audio.Loop = defaults.Loop;
            audio.PlayOnStart = defaults.PlayOnStart;
            // Source préservée, comme les handles Box2D : c'est la lecture en cours.
            RecordEdit(entity, before, "reset Audio of");
        }

        if (open)
        {
            const Engine::AudioComponent audioBeforeDrop = audio;

            char pathBuffer[512];
            memset(pathBuffer, 0, sizeof(pathBuffer));
            strncpy(pathBuffer, Engine::AssetManager::GetPath(audio.Sound).c_str(), sizeof(pathBuffer) - 1);
            if (ImGui::InputText("File", pathBuffer, sizeof(pathBuffer)))
            {
                const std::string path(pathBuffer);
                audio.Sound = path.empty() ? Engine::AssetHandle(Engine::k_InvalidAssetHandle)
                                           : Engine::AssetManager::Import(path);
                // Le son déjà chargé ne correspond plus à la référence affichée.
                audio.Source.reset();
            }
            MarkOverride("AudioComponent", "Sound");
            if (AcceptAssetDrop(Engine::AssetType::Audio, audio.Sound))
            {
                audio.Source.reset();
                RecordEdit(entity, audioBeforeDrop, "change sound of");
            }
            TrackEdit<Engine::AudioComponent>(entity, "change sound of");

            ImGui::DragFloat("Volume", &audio.Volume, 0.01f, 0.0f, 1.0f, "%.2f", k_ClampedDrag);
            MarkOverride("AudioComponent", "Volume");
            TrackEdit<Engine::AudioComponent>(entity, "change volume of");

            ImGui::Checkbox("Loop", &audio.Loop);
            MarkOverride("AudioComponent", "Loop");
            TrackEdit<Engine::AudioComponent>(entity, "edit Audio of");

            ImGui::Checkbox("Play On Start", &audio.PlayOnStart);
            MarkOverride("AudioComponent", "PlayOnStart");
            TrackEdit<Engine::AudioComponent>(entity, "edit Audio of");

            // Écoute depuis l'éditeur, sans passer par le Play : le son est chargé à la
            // demande au premier clic.
            const bool playing = audio.Source && audio.Source->IsPlaying();
            if (ImGui::Button(playing ? "Stop" : "Play"))
            {
                if (playing)
                {
                    audio.Source->Stop();
                }
                else if (Engine::AssetManager::IsValid(audio.Sound))
                {
                    if (!audio.Source)
                        audio.Source = Engine::AssetManager::LoadSound(audio.Sound);
                    audio.Source->SetVolume(audio.Volume);
                    audio.Source->SetLooping(audio.Loop);
                    audio.Source->Play();
                }
            }

            if (audio.Source && !audio.Source->IsLoaded())
            {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "File not found");
            }
        }

        EndComponentSection(open);
    }

    if (entity.HasComponent<Engine::CameraComponent>())
    {
        bool reset = false;
        const bool open = BeginComponentSection(entity, "Camera", "CameraComponent", reset);

        auto &cc = entity.GetComponent<Engine::CameraComponent>();
        if (reset)
        {
            // Camera (l'OrthographicCamera embarquée) est reconstruite à chaque rendu
            // par GamePanel, rien à réinitialiser dessus.
            const Engine::CameraComponent before = cc;
            const Engine::CameraComponent defaults;
            cc.Primary = defaults.Primary;
            cc.OrthographicSize = defaults.OrthographicSize;
            RecordEdit(entity, before, "reset Camera of");
        }

        if (open)
        {
            ImGui::Checkbox("Primary", &cc.Primary);
            MarkOverride("CameraComponent", "Primary");
            TrackEdit<Engine::CameraComponent>(entity, "edit Camera of");
            ImGui::DragFloat("Orthographic Size", &cc.OrthographicSize, 0.05f, 0.05f, 100.0f, "%.3f", k_ClampedDrag);
            MarkOverride("CameraComponent", "OrthographicSize");
            TrackEdit<Engine::CameraComponent>(entity, "edit Camera of");
        }

        EndComponentSection(open);
    }
}

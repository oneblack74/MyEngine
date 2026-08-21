#include "Panels/InspectorPanel.h"
#include "Panels/ContentBrowserPanel.h"
#include <Assets/AssetManager.h>
#include <filesystem>
#include <Scene/Components.h>
#include <imgui.h>
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

    // En-tête d'une section de component : le TreeNode habituel plus un bouton
    // "Reset" aligné à droite de la même ligne. Renvoie true si la section est
    // ouverte ; resetRequested passe à true la frame où le bouton est cliqué.
    // À refermer avec EndComponentSection().
    bool BeginComponentSection(const char *label, bool &resetRequested)
    {
        const ImGuiStyle &style = ImGui::GetStyle();
        const float buttonWidth = ImGui::CalcTextSize("Reset").x + style.FramePadding.x * 2.0f;
        const float lineStartX = ImGui::GetCursorPosX();
        const float availWidth = ImGui::GetContentRegionAvail().x;

        // AllowOverlap : sans ça le TreeNode capterait le clic destiné au bouton posé
        // par-dessus sa ligne.
        const bool open = ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowOverlap);

        // Le TreeNode ouvert empile déjà son propre ID, ce qui suffit à distinguer les
        // boutons entre sections ; replié il n'empile rien, d'où le suffixe ## explicite.
        // (Un PushID(label) autour de tout donnerait un ID en double, "Transform/Transform",
        // qui alourdirait les chemins utilisés par les tests automatisés.)
        char buttonId[64];
        snprintf(buttonId, sizeof(buttonId), "Reset##%s", label);

        ImGui::SameLine(lineStartX + availWidth - buttonWidth);
        resetRequested = ImGui::SmallButton(buttonId);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Reset %s to its default values", label);

        return open;
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

        if (ImGui::IsItemActivated())
            m_NameBeforeEdit = tag;
        if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
            tag = std::string(buffer);
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
        const bool open = BeginComponentSection("Transform", reset);

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
            TrackEdit<Engine::TransformComponent>(entity, "move");
            ImGui::DragFloat("Rotation", &transform.Rotation, 0.5f);
            TrackEdit<Engine::TransformComponent>(entity, "rotate");
            ImGui::DragFloat3("Scale", &transform.Scale.x, 0.05f);
            TrackEdit<Engine::TransformComponent>(entity, "scale");
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
            if (AcceptAssetDrop(Engine::AssetType::Texture, sprite.Texture))
                RecordEdit(entity, spriteBeforeDrop, "change texture of");
            TrackEdit<Engine::SpriteRendererComponent>(entity, "change texture of");

            ImGui::ColorEdit4("Color", &sprite.Color.x);
            TrackEdit<Engine::SpriteRendererComponent>(entity, "change color of");
            ImGui::DragFloat("Tiling", &sprite.TilingFactor, 0.05f, 0.0f, 100.0f, "%.3f", k_ClampedDrag);
            TrackEdit<Engine::SpriteRendererComponent>(entity, "change tiling of");
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

            ImGui::Checkbox("Fixed Rotation", &rb.FixedRotation);
            TrackEdit<Engine::RigidBodyComponent>(entity, "edit Rigid Body of");
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
            TrackEdit<Engine::BoxColliderComponent>(entity, "edit collider of");
            ImGui::DragFloat2("Taille (demi)", &bc.Size.x, 0.05f, 0.01f, 100.0f, "%.3f", k_ClampedDrag);
            TrackEdit<Engine::BoxColliderComponent>(entity, "edit collider of");
            ImGui::DragFloat("Density", &bc.Density, 0.05f, 0.0f, 100.0f, "%.3f", k_ClampedDrag);
            TrackEdit<Engine::BoxColliderComponent>(entity, "edit collider of");
            ImGui::DragFloat("Friction", &bc.Friction, 0.01f, 0.0f, 1.0f, "%.3f", k_ClampedDrag);
            TrackEdit<Engine::BoxColliderComponent>(entity, "edit collider of");
            ImGui::DragFloat("Restitution", &bc.Restitution, 0.01f, 0.0f, 1.0f, "%.3f", k_ClampedDrag);
            TrackEdit<Engine::BoxColliderComponent>(entity, "edit collider of");
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
            TrackEdit<Engine::CircleColliderComponent>(entity, "edit collider of");
            ImGui::DragFloat("Radius", &cc.Radius, 0.05f, 0.01f, 100.0f, "%.3f", k_ClampedDrag);
            TrackEdit<Engine::CircleColliderComponent>(entity, "edit collider of");
            ImGui::DragFloat("Density", &cc.Density, 0.05f, 0.0f, 100.0f, "%.3f", k_ClampedDrag);
            TrackEdit<Engine::CircleColliderComponent>(entity, "edit collider of");
            ImGui::DragFloat("Friction", &cc.Friction, 0.01f, 0.0f, 1.0f, "%.3f", k_ClampedDrag);
            TrackEdit<Engine::CircleColliderComponent>(entity, "edit collider of");
            ImGui::DragFloat("Restitution", &cc.Restitution, 0.01f, 0.0f, 1.0f, "%.3f", k_ClampedDrag);
            TrackEdit<Engine::CircleColliderComponent>(entity, "edit collider of");
        }

        EndComponentSection(open);
    }

    if (entity.HasComponent<Engine::AudioComponent>())
    {
        bool reset = false;
        const bool open = BeginComponentSection("Audio", reset);

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
            if (AcceptAssetDrop(Engine::AssetType::Audio, audio.Sound))
            {
                audio.Source.reset();
                RecordEdit(entity, audioBeforeDrop, "change sound of");
            }
            TrackEdit<Engine::AudioComponent>(entity, "change sound of");

            ImGui::DragFloat("Volume", &audio.Volume, 0.01f, 0.0f, 1.0f, "%.2f", k_ClampedDrag);
            TrackEdit<Engine::AudioComponent>(entity, "change volume of");

            ImGui::Checkbox("Loop", &audio.Loop);
            TrackEdit<Engine::AudioComponent>(entity, "edit Audio of");

            ImGui::Checkbox("Play On Start", &audio.PlayOnStart);
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
        const bool open = BeginComponentSection("Camera", reset);

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
            TrackEdit<Engine::CameraComponent>(entity, "edit Camera of");
            ImGui::DragFloat("Orthographic Size", &cc.OrthographicSize, 0.05f, 0.05f, 100.0f, "%.3f", k_ClampedDrag);
            TrackEdit<Engine::CameraComponent>(entity, "edit Camera of");
        }

        EndComponentSection(open);
    }
}

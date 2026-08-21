#include "Scene/SceneSerializer.h"
#include "Scene/Entity.h"
#include "Scene/Components.h"
#include "Core/Log.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>

using json = nlohmann::json;

namespace Engine
{
    SceneSerializer::SceneSerializer(const std::shared_ptr<Scene> &scene)
        : m_Scene(scene)
    {
    }

    void SceneSerializer::Serialize(const std::string &filepath)
    {
        std::ofstream file(filepath);
        file << ToJson().dump(4);
    }

    json SceneSerializer::ToJson() const
    {
        json root;
        root["Entities"] = json::array();

        auto view = m_Scene->GetAllEntitiesWith<IDComponent>();
        for (auto entityHandle : view)
        {
            Entity entity = {entityHandle, m_Scene.get()};

            json entityJson;
            entityJson["UUID"] = (uint64_t)entity.GetUUID();

            if (entity.HasComponent<TagComponent>())
                entityJson["TagComponent"]["Tag"] = entity.GetComponent<TagComponent>().Tag;

            // Le parent n'est écrit que s'il y en a un : une entité racine reste un
            // objet JSON propre, sans champ "Parent": 0 partout.
            if (entity.HasComponent<ParentComponent>())
            {
                const UUID parent = entity.GetComponent<ParentComponent>().Parent;
                if ((uint64_t)parent != 0)
                    entityJson["Parent"] = (uint64_t)parent;
            }

            if (entity.HasComponent<TransformComponent>())
            {
                auto &tc = entity.GetComponent<TransformComponent>();
                entityJson["TransformComponent"]["Position"] = {tc.Position.x, tc.Position.y, tc.Position.z};
                entityJson["TransformComponent"]["Rotation"] = tc.Rotation;
                entityJson["TransformComponent"]["Scale"] = {tc.Scale.x, tc.Scale.y, tc.Scale.z};
            }

            if (entity.HasComponent<SpriteRendererComponent>())
            {
                auto &sc = entity.GetComponent<SpriteRendererComponent>();
                entityJson["SpriteRendererComponent"]["Color"] = {sc.Color.r, sc.Color.g, sc.Color.b, sc.Color.a};
                entityJson["SpriteRendererComponent"]["TilingFactor"] = sc.TilingFactor;
                // La texture est enfin sauvegardée : c'est une référence d'asset stable,
                // là où le shared_ptr d'avant ne pouvait pas être écrit dans un fichier.
                entityJson["SpriteRendererComponent"]["Texture"] = (uint64_t)sc.Texture;
                // La texture n'est pas sérialisée : il n'y a pas encore de système d'assets
                // (chemin -> handle stable) pour la référencer de façon fiable. Prévu en Phase 7.
            }

            if (entity.HasComponent<RigidBodyComponent>())
            {
                auto &rb = entity.GetComponent<RigidBodyComponent>();
                entityJson["RigidBodyComponent"]["Type"] = (int)rb.Type;
                entityJson["RigidBodyComponent"]["FixedRotation"] = rb.FixedRotation;
                // RuntimeBody n'est pas sérialisé : c'est un handle Box2D valide seulement
                // pendant le Play, recréé par PhysicsSystem à chaque fois.
            }

            if (entity.HasComponent<BoxColliderComponent>())
            {
                auto &bc = entity.GetComponent<BoxColliderComponent>();
                entityJson["BoxColliderComponent"]["Offset"] = {bc.Offset.x, bc.Offset.y};
                entityJson["BoxColliderComponent"]["Size"] = {bc.Size.x, bc.Size.y};
                entityJson["BoxColliderComponent"]["Density"] = bc.Density;
                entityJson["BoxColliderComponent"]["Friction"] = bc.Friction;
                entityJson["BoxColliderComponent"]["Restitution"] = bc.Restitution;
            }

            if (entity.HasComponent<CircleColliderComponent>())
            {
                auto &cc = entity.GetComponent<CircleColliderComponent>();
                entityJson["CircleColliderComponent"]["Offset"] = {cc.Offset.x, cc.Offset.y};
                entityJson["CircleColliderComponent"]["Radius"] = cc.Radius;
                entityJson["CircleColliderComponent"]["Density"] = cc.Density;
                entityJson["CircleColliderComponent"]["Friction"] = cc.Friction;
                entityJson["CircleColliderComponent"]["Restitution"] = cc.Restitution;
            }

            if (entity.HasComponent<AudioComponent>())
            {
                auto &ac = entity.GetComponent<AudioComponent>();
                entityJson["AudioComponent"]["Sound"] = (uint64_t)ac.Sound;
                entityJson["AudioComponent"]["Volume"] = ac.Volume;
                entityJson["AudioComponent"]["Loop"] = ac.Loop;
                entityJson["AudioComponent"]["PlayOnStart"] = ac.PlayOnStart;
            }

            if (entity.HasComponent<SceneInstanceComponent>())
            {
                auto &si = entity.GetComponent<SceneInstanceComponent>();
                entityJson["SceneInstanceComponent"]["Source"] = (uint64_t)si.Source;
            }

            if (entity.HasComponent<SceneInstanceMemberComponent>())
            {
                auto &member = entity.GetComponent<SceneInstanceMemberComponent>();
                entityJson["SceneInstanceMemberComponent"]["SourceEntity"] = (uint64_t)member.SourceEntity;
            }

            if (entity.HasComponent<CameraComponent>())
            {
                auto &cc = entity.GetComponent<CameraComponent>();
                entityJson["CameraComponent"]["OrthographicSize"] = cc.OrthographicSize;
                entityJson["CameraComponent"]["Primary"] = cc.Primary;
                // Camera (OrthographicCamera) n'est pas sérialisée : c'est un objet de
                // travail recalculé à chaque rendu depuis OrthographicSize, pas un état.
            }

            root["Entities"].push_back(entityJson);
        }

        return root;
    }

    bool SceneSerializer::Deserialize(const std::string &filepath)
    {
        std::ifstream file(filepath);
        if (!file.is_open())
        {
            LOG_ERROR("SceneSerializer: could not open {0}", filepath);
            return false;
        }

        json root;
        file >> root;

        if (!FromJson(root))
            return false;

        // Une scène a exactement une racine. Un fichier écrit avant cette règle en a
        // plusieurs : elles sont regroupées sous une nouvelle entité portant le nom du
        // fichier, plutôt que de refuser de charger la scène.
        m_Scene->EnsureSingleRoot(std::filesystem::path(filepath).stem().string());
        return true;
    }

    bool SceneSerializer::FromJson(const json &root)
    {
        if (!root.contains("Entities"))
            return false;


        for (auto &entityJson : root["Entities"])
        {
            std::string name;
            if (entityJson.contains("TagComponent"))
                name = entityJson["TagComponent"]["Tag"].get<std::string>();

            // L'UUID sauvegardé est passé à la création, et non écrit dans l'IDComponent
            // après coup : la scène indexe ses entités par UUID au moment où elle les
            // crée, un remplacement tardif lui laisserait une clé périmée.
            const uint64_t uuid = entityJson["UUID"].get<uint64_t>();
            Entity entity = m_Scene->CreateEntityWithUUID(UUID(uuid), name);

            // Le parent est référencé par UUID, donc l'ordre du fichier n'a pas
            // d'importance : il sera résolu à la première consultation de la hiérarchie.
            if (entityJson.contains("Parent"))
                entity.GetComponent<ParentComponent>().Parent = UUID(entityJson["Parent"].get<uint64_t>());

            if (entityJson.contains("TransformComponent"))
            {
                auto &tc = entity.GetComponent<TransformComponent>();
                auto pos = entityJson["TransformComponent"]["Position"];
                tc.Position = {pos[0], pos[1], pos[2]};
                tc.Rotation = entityJson["TransformComponent"]["Rotation"].get<float>();
                auto scale = entityJson["TransformComponent"]["Scale"];
                tc.Scale = {scale[0], scale[1], scale[2]};
            }

            if (entityJson.contains("SpriteRendererComponent"))
            {
                auto &sc = entity.AddComponent<SpriteRendererComponent>();
                auto color = entityJson["SpriteRendererComponent"]["Color"];
                sc.Color = {color[0], color[1], color[2], color[3]};
                sc.TilingFactor = entityJson["SpriteRendererComponent"]["TilingFactor"].get<float>();
                // Absente des scènes enregistrées avant l'arrivée du système d'assets.
                if (entityJson["SpriteRendererComponent"].contains("Texture"))
                    sc.Texture = entityJson["SpriteRendererComponent"]["Texture"].get<uint64_t>();
            }

            if (entityJson.contains("RigidBodyComponent"))
            {
                auto &rb = entity.AddComponent<RigidBodyComponent>();
                rb.Type = (RigidBodyComponent::BodyType)entityJson["RigidBodyComponent"]["Type"].get<int>();
                rb.FixedRotation = entityJson["RigidBodyComponent"]["FixedRotation"].get<bool>();
            }

            if (entityJson.contains("BoxColliderComponent"))
            {
                auto &bc = entity.AddComponent<BoxColliderComponent>();
                auto offset = entityJson["BoxColliderComponent"]["Offset"];
                bc.Offset = {offset[0], offset[1]};
                auto size = entityJson["BoxColliderComponent"]["Size"];
                bc.Size = {size[0], size[1]};
                bc.Density = entityJson["BoxColliderComponent"]["Density"].get<float>();
                bc.Friction = entityJson["BoxColliderComponent"]["Friction"].get<float>();
                bc.Restitution = entityJson["BoxColliderComponent"]["Restitution"].get<float>();
            }

            if (entityJson.contains("CircleColliderComponent"))
            {
                auto &cc = entity.AddComponent<CircleColliderComponent>();
                auto offset = entityJson["CircleColliderComponent"]["Offset"];
                cc.Offset = {offset[0], offset[1]};
                cc.Radius = entityJson["CircleColliderComponent"]["Radius"].get<float>();
                cc.Density = entityJson["CircleColliderComponent"]["Density"].get<float>();
                cc.Friction = entityJson["CircleColliderComponent"]["Friction"].get<float>();
                cc.Restitution = entityJson["CircleColliderComponent"]["Restitution"].get<float>();
            }

            if (entityJson.contains("AudioComponent"))
            {
                auto &ac = entity.AddComponent<AudioComponent>();
                ac.Sound = entityJson["AudioComponent"]["Sound"].get<uint64_t>();
                ac.Volume = entityJson["AudioComponent"]["Volume"].get<float>();
                ac.Loop = entityJson["AudioComponent"]["Loop"].get<bool>();
                ac.PlayOnStart = entityJson["AudioComponent"]["PlayOnStart"].get<bool>();
            }

            if (entityJson.contains("SceneInstanceComponent"))
            {
                auto &si = entity.AddComponent<SceneInstanceComponent>();
                si.Source = entityJson["SceneInstanceComponent"]["Source"].get<uint64_t>();
            }

            if (entityJson.contains("SceneInstanceMemberComponent"))
            {
                auto &member = entity.AddComponent<SceneInstanceMemberComponent>();
                member.SourceEntity = entityJson["SceneInstanceMemberComponent"]["SourceEntity"].get<uint64_t>();
            }

            if (entityJson.contains("CameraComponent"))
            {
                auto &cc = entity.AddComponent<CameraComponent>();
                cc.OrthographicSize = entityJson["CameraComponent"]["OrthographicSize"].get<float>();
                cc.Primary = entityJson["CameraComponent"]["Primary"].get<bool>();
            }
        }

        return true;
    }
}

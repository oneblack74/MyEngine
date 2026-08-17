#include "Scene/SceneSerializer.h"
#include "Scene/Entity.h"
#include "Scene/Components.h"
#include "Core/Log.h"
#include <nlohmann/json.hpp>
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

            root["Entities"].push_back(entityJson);
        }

        std::ofstream file(filepath);
        file << root.dump(4);
    }

    bool SceneSerializer::Deserialize(const std::string &filepath)
    {
        std::ifstream file(filepath);
        if (!file.is_open())
        {
            LOG_ERROR("SceneSerializer: impossible d'ouvrir {0}", filepath);
            return false;
        }

        json root;
        file >> root;

        if (!root.contains("Entities"))
            return false;

        for (auto &entityJson : root["Entities"])
        {
            std::string name;
            if (entityJson.contains("TagComponent"))
                name = entityJson["TagComponent"]["Tag"].get<std::string>();

            Entity entity = m_Scene->CreateEntity(name);

            // CreateEntity génère un nouvel UUID aléatoire : on le remplace par celui
            // sauvegardé pour préserver l'identité de l'entité entre les sessions.
            uint64_t uuid = entityJson["UUID"].get<uint64_t>();
            entity.GetComponent<IDComponent>().ID = UUID(uuid);

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
        }

        return true;
    }
}

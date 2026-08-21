#pragma once
#include "Scene/Entity.h"
#include "Scene/Scene.h"
#include <nlohmann/json_fwd.hpp>
#include <memory>
#include <string>

namespace Engine
{
    // Extension des fichiers de scène. Le contenu est du JSON, mais l'extension est
    // propre au moteur : elle distingue une scène du registre d'assets et de tout
    // autre .json qui traînerait dans le dossier.
    inline constexpr const char *k_SceneExtension = ".scene";

    // Sauvegarde/charge une Scene (entités + components) au format JSON.
    class SceneSerializer
    {
    public:
        SceneSerializer(const std::shared_ptr<Scene> &scene);

        void Serialize(const std::string &filepath);
        bool Deserialize(const std::string &filepath);

        // Mêmes conversions, sans passer par le disque. C'est cette forme qu'utilise la
        // fusion des instances : le JSON nomme chaque propriété, ce qui permet de
        // comparer et fusionner sans avoir à réénumérer les champs des components.
        nlohmann::json ToJson() const;
        bool FromJson(const nlohmann::json &root);

        // Une entité seule, dans les deux sens. L'éditeur s'en sert pour comparer une
        // instance à sa scène source propriété par propriété, et pour révoquer une
        // surcharge en réécrivant la valeur d'origine.
        static nlohmann::json EntityToJson(Entity entity);
        static void ApplyJsonToEntity(const nlohmann::json &entityJson, Entity entity);

    private:
        std::shared_ptr<Scene> m_Scene;
    };
}

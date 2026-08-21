#pragma once
#include "Scene/Scene.h"
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

    private:
        std::shared_ptr<Scene> m_Scene;
    };
}

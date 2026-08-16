#pragma once
#include "Scene/Scene.h"
#include <memory>
#include <string>

namespace Engine
{
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

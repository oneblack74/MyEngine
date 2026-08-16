#pragma once
#include "Scene/Scene.h"
#include <memory>
#include <string>

namespace Engine
{
    // Centralise la notion de "scène active" et les opérations load/save/switch dessus.
    class SceneManager
    {
    public:
        static std::shared_ptr<Scene> NewScene();
        static std::shared_ptr<Scene> LoadScene(const std::string &filepath);
        static void SaveActiveScene(const std::string &filepath);

        static void SetActiveScene(const std::shared_ptr<Scene> &scene);
        static const std::shared_ptr<Scene> &GetActiveScene();

    private:
        static std::shared_ptr<Scene> s_ActiveScene;
    };
}

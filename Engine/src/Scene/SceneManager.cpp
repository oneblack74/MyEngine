#include "Scene/SceneManager.h"
#include "Scene/SceneSerializer.h"
#include "Core/Log.h"

namespace Engine
{
    std::shared_ptr<Scene> SceneManager::s_ActiveScene = nullptr;

    std::shared_ptr<Scene> SceneManager::NewScene()
    {
        s_ActiveScene = std::make_shared<Scene>();
        return s_ActiveScene;
    }

    std::shared_ptr<Scene> SceneManager::LoadScene(const std::string &filepath)
    {
        auto scene = std::make_shared<Scene>();
        SceneSerializer serializer(scene);
        if (!serializer.Deserialize(filepath))
        {
            LOG_ERROR("SceneManager: échec du chargement de {0}", filepath);
            return nullptr;
        }

        s_ActiveScene = scene;
        return s_ActiveScene;
    }

    void SceneManager::SaveActiveScene(const std::string &filepath)
    {
        if (!s_ActiveScene)
        {
            LOG_WARN("SceneManager: aucune scène active à sauvegarder");
            return;
        }

        SceneSerializer serializer(s_ActiveScene);
        serializer.Serialize(filepath);
    }

    void SceneManager::SetActiveScene(const std::shared_ptr<Scene> &scene)
    {
        s_ActiveScene = scene;
    }

    const std::shared_ptr<Scene> &SceneManager::GetActiveScene()
    {
        return s_ActiveScene;
    }
}

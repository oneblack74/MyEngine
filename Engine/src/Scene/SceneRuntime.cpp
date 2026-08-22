#include "Scene/SceneRuntime.h"
#include "Core/Log.h"
#include "Renderer/Renderer.h"
#include "Scene/AudioSystem.h"
#include "Scene/Components.h"
#include "Scene/RenderSystem.h"
#include "Scene/SceneManager.h"

namespace Engine
{
    SceneRuntime::~SceneRuntime()
    {
        Stop();
    }

    bool SceneRuntime::Start(const std::string &scenePath)
    {
        std::shared_ptr<Scene> scene = SceneManager::LoadScene(scenePath);
        if (!scene)
            return false;

        Start(scene);
        return true;
    }

    void SceneRuntime::Start(const std::shared_ptr<Scene> &scene)
    {
        Stop();

        // La scène donnée est jouée telle quelle, sans la copie que fait l'éditeur au
        // Play : hors éditeur il n'y a pas de version « d'édition » à préserver derrière.
        m_Scene = scene;
        SceneManager::SetActiveScene(m_Scene);

        m_PhysicsSystem.OnRuntimeStart(*m_Scene);
        AudioSystem::OnRuntimeStart(*m_Scene);
    }

    void SceneRuntime::Stop()
    {
        if (!m_Scene)
            return;

        m_PhysicsSystem.OnRuntimeStop();
        AudioSystem::OnRuntimeStop(*m_Scene);
        m_Scene = nullptr;
    }

    void SceneRuntime::OnUpdate(float timestep)
    {
        if (!m_Scene)
            return;

        m_PhysicsSystem.OnUpdate(*m_Scene, timestep);
        AudioSystem::OnUpdate(*m_Scene);
    }

    Entity SceneRuntime::GetPrimaryCamera()
    {
        if (!m_Scene)
            return {};

        auto view = m_Scene->GetAllEntitiesWith<CameraComponent>();
        for (auto entityHandle : view)
        {
            Entity entity{entityHandle, m_Scene.get()};
            if (entity.GetComponent<CameraComponent>().Primary)
                return entity;
        }
        return {};
    }

    void SceneRuntime::Render(uint32_t width, uint32_t height)
    {
        if (!m_Scene || width == 0 || height == 0) // fenêtre minimisée
            return;

        Renderer::SetViewport(0, 0, width, height);
        Renderer::SetClearColor(ClearColor.r, ClearColor.g, ClearColor.b, ClearColor.a);
        Renderer::Clear();

        Entity camera = GetPrimaryCamera();
        if (!camera)
        {
            if (!m_WarnedNoCamera)
            {
                LOG_WARN("SceneRuntime: the scene has no primary camera, nothing to show");
                m_WarnedNoCamera = true;
            }
            return;
        }

        // La projection suit le ratio de la cible et la demi-hauteur voulue ; la
        // position vient du transform monde — une caméra enfant d'une autre entité doit
        // suivre son parent.
        auto &cameraComponent = camera.GetComponent<CameraComponent>();
        const TransformComponent transform = m_Scene->GetWorldTransform(camera);
        const float aspectRatio = (float)width / (float)height;
        const float size = cameraComponent.OrthographicSize;
        cameraComponent.Camera.SetProjection(-aspectRatio * size, aspectRatio * size, -size, size);
        cameraComponent.Camera.SetPosition(transform.Position);
        cameraComponent.Camera.SetRotation(transform.Rotation);

        RenderSystem::Render(*m_Scene, cameraComponent.Camera);
    }
}

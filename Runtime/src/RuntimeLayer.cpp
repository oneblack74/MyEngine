#include "RuntimeLayer.h"
#include <Assets/AssetManager.h>
#include <Core/Application.h>
#include <Core/KeyCodes.h>
#include <Core/Log.h>
#include <Events/KeyEvent.h>
#include <Renderer/Renderer.h>
#include <Renderer/Renderer2D.h>
#include <Scene/AudioSystem.h>
#include <Scene/Components.h>
#include <Scene/Entity.h>
#include <Scene/RenderSystem.h>
#include <Scene/SceneManager.h>
#include <Utils/ImageCapture.h>
#include <filesystem>

namespace
{
    Engine::Entity FindPrimaryCamera(Engine::Scene &scene)
    {
        auto view = scene.GetAllEntitiesWith<Engine::CameraComponent>();
        for (auto entityHandle : view)
        {
            Engine::Entity entity{entityHandle, &scene};
            if (entity.GetComponent<Engine::CameraComponent>().Primary)
                return entity;
        }
        return {};
    }
}

RuntimeLayer::RuntimeLayer(const RuntimeOptions &options)
    : Layer("RuntimeLayer"), m_Options(options)
{
}

void RuntimeLayer::OnAttach()
{
    Engine::Renderer2D::Init();

    // Le chemin de scène est relatif à la racine des assets ; operator/ laisse passer
    // tel quel un chemin déjà absolu.
    const std::filesystem::path scenePath = Engine::AssetManager::GetAssetRoot() / m_Options.ScenePath;
    m_Scene = Engine::SceneManager::LoadScene(scenePath.string());
    if (!m_Scene)
    {
        LOG_ERROR("Runtime: no scene to play, giving up");
        m_ExitCode = 1;
        Engine::Application::Get().Close();
        return;
    }

    LOG_INFO("Runtime: playing {0}", scenePath.string());

    // Pas d'équivalent du Play de l'éditeur : la scène chargée EST la scène de jeu, il
    // n'y a rien à préserver derrière elle, donc pas de copie.
    m_PhysicsSystem.OnRuntimeStart(*m_Scene);
    Engine::AudioSystem::OnRuntimeStart(*m_Scene);
}

void RuntimeLayer::OnDetach()
{
    if (m_Scene)
    {
        m_PhysicsSystem.OnRuntimeStop();
        Engine::AudioSystem::OnRuntimeStop(*m_Scene);
    }

    Engine::Renderer2D::Shutdown();
}

void RuntimeLayer::OnUpdate(Engine::Timestep ts)
{
    if (!m_Scene)
        return;

    m_PhysicsSystem.OnUpdate(*m_Scene, ts);
    Engine::AudioSystem::OnUpdate(*m_Scene);

    RenderFrame();
    ++m_FrameCount;

    // Avant le swap de Window::OnUpdate : après, le back buffer ne contient plus ce
    // qu'on vient de dessiner.
    const bool lastFrame = m_Options.MaxFrames > 0 && m_FrameCount >= m_Options.MaxFrames;
    if (!m_Options.ScreenshotPath.empty() && lastFrame)
    {
        Engine::Window &window = Engine::Application::Get().GetWindow();
        if (Engine::ImageCapture::CaptureBackBuffer(m_Options.ScreenshotPath, window.GetWidth(), window.GetHeight()))
            LOG_INFO("Runtime: screenshot written to {0}", m_Options.ScreenshotPath);
        else
        {
            LOG_ERROR("Runtime: could not write screenshot to {0}", m_Options.ScreenshotPath);
            m_ExitCode = 1;
        }
    }

    if (lastFrame)
        Engine::Application::Get().Close();
}

void RuntimeLayer::OnEvent(Engine::Event &event)
{
    // Un jeu standalone n'a pas de menu : Échap est la seule sortie au clavier.
    if (event.GetType() == Engine::EventType::KeyPressed &&
        static_cast<Engine::KeyPressedEvent &>(event).GetKeyCode() == Engine::Key::Escape)
    {
        Engine::Application::Get().Close();
        event.Handled = true;
    }
}

void RuntimeLayer::RenderFrame()
{
    Engine::Window &window = Engine::Application::Get().GetWindow();
    const uint32_t width = window.GetWidth();
    const uint32_t height = window.GetHeight();
    if (width == 0 || height == 0) // fenêtre minimisée
        return;

    // Rendu direct dans la fenêtre, pas dans un Framebuffer : personne n'a besoin de
    // l'image en tant que texture ici, contrairement aux panels de l'éditeur.
    Engine::Renderer::SetViewport(0, 0, width, height);
    Engine::Renderer::SetClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    Engine::Renderer::Clear();

    Engine::Entity camera = FindPrimaryCamera(*m_Scene);
    if (!camera)
    {
        if (!m_WarnedNoCamera)
        {
            LOG_WARN("Runtime: the scene has no primary camera, nothing to show");
            m_WarnedNoCamera = true;
        }
        return;
    }

    // Même règle que le panel Game de l'éditeur : la projection suit le ratio de la
    // cible (ici la fenêtre) et la demi-hauteur voulue, la position vient du transform
    // monde — une caméra enfant d'une autre entité doit suivre son parent.
    auto &cameraComponent = camera.GetComponent<Engine::CameraComponent>();
    const Engine::TransformComponent transform = m_Scene->GetWorldTransform(camera);
    const float aspectRatio = (float)width / (float)height;
    const float size = cameraComponent.OrthographicSize;
    cameraComponent.Camera.SetProjection(-aspectRatio * size, aspectRatio * size, -size, size);
    cameraComponent.Camera.SetPosition(transform.Position);
    cameraComponent.Camera.SetRotation(transform.Rotation);

    Engine::RenderSystem::Render(*m_Scene, cameraComponent.Camera);
}

#include "RuntimeLayer.h"
#include <Assets/AssetManager.h>
#include <Core/Application.h>
#include <Core/KeyCodes.h>
#include <Core/Log.h>
#include <Events/KeyEvent.h>
#include <Renderer/Renderer2D.h>
#include <Utils/ImageCapture.h>
#include <filesystem>

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
    if (!m_SceneRuntime.Start(scenePath.string()))
    {
        LOG_ERROR("Runtime: no scene to play, giving up");
        m_ExitCode = 1;
        Engine::Application::Get().Close();
        return;
    }

    LOG_INFO("Runtime: playing {0}", scenePath.string());
}

void RuntimeLayer::OnDetach()
{
    m_SceneRuntime.Stop();
    Engine::Renderer2D::Shutdown();
}

void RuntimeLayer::OnUpdate(Engine::Timestep ts)
{
    if (!m_SceneRuntime.GetScene())
        return;

    Engine::Window &window = Engine::Application::Get().GetWindow();

    m_SceneRuntime.OnUpdate(ts);
    m_SceneRuntime.Render(window.GetWidth(), window.GetHeight());
    ++m_FrameCount;

    // Avant le swap de Window::OnUpdate : après, le back buffer ne contient plus ce
    // qu'on vient de dessiner.
    const bool lastFrame = m_Options.MaxFrames > 0 && m_FrameCount >= m_Options.MaxFrames;
    if (!m_Options.ScreenshotPath.empty() && lastFrame)
    {
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

#include "GameLayer.h"
#include <Assets/AssetManager.h>
#include <Audio/AudioSource.h>
#include <Core/Application.h>
#include <Core/Input.h>
#include <Core/KeyCodes.h>
#include <Core/Log.h>
#include <Events/KeyEvent.h>
#include <Renderer/Renderer2D.h>
#include <Scene/Components.h>
#include <Utils/ImageCapture.h>
#include <algorithm>
#include <filesystem>

namespace
{
    constexpr const char *k_ScenePath = "scenes/catch.scene";
    constexpr const char *k_CrateTag = "Crate";
    constexpr const char *k_MarkerTag = "Marker";

    constexpr int k_StartingLives = 3;
    constexpr float k_BasketSpeed = 2.4f;
    constexpr float k_BasketHalfWidth = 0.18f;   // moitié du Scale.x du panier dans la scène
    constexpr float k_PlayfieldHalfWidth = 1.5f; // au-delà, on sort du champ de la caméra
    constexpr float k_SpawnInterval = 0.9f;
    constexpr float k_SpawnHeight = 1.15f;
    constexpr float k_KillLine = -1.25f; // sous le panier : la caisse est perdue

    constexpr float k_MarkerSize = 0.05f;
    constexpr float k_MarkerSpacing = 0.075f;
    constexpr float k_MarkerRowY = 0.85f;
    constexpr int k_MarkersPerRow = 20;

    const glm::vec4 k_ScoreColor{0.4f, 0.85f, 0.5f, 1.0f};
    const glm::vec4 k_LifeColor{0.9f, 0.35f, 0.35f, 1.0f};
    const glm::vec4 k_CrateColor{0.9f, 0.6f, 0.25f, 1.0f};
}

GameLayer::GameLayer(const GameOptions &options)
    : Layer("GameLayer"), m_Options(options)
{
}

void GameLayer::OnAttach()
{
    Engine::Renderer2D::Init();

    // Le PhysicsSystem survit aux parties (il appartient au SceneRuntime) : le hook est
    // posé une fois pour toutes, pas à chaque StartRound.
    m_SceneRuntime.GetPhysics().OnCollisionBegin = [this](Engine::Entity a, Engine::Entity b)
    {
        Engine::Entity crate;
        if (a == m_Basket && b.GetName() == k_CrateTag)
            crate = b;
        else if (b == m_Basket && a.GetName() == k_CrateTag)
            crate = a;
        else
            return;

        m_CaughtCrates.push_back(crate.GetUUID());
    };

    StartRound();
}

void GameLayer::OnDetach()
{
    m_SceneRuntime.Stop();
    Engine::Renderer2D::Shutdown();
}

void GameLayer::StartRound()
{
    const std::filesystem::path scenePath = Engine::AssetManager::GetAssetRoot() / k_ScenePath;
    if (!m_SceneRuntime.Start(scenePath.string()))
    {
        LOG_ERROR("MyFirstGame: could not load {0}", scenePath.string());
        m_ExitCode = 1;
        Engine::Application::Get().Close();
        return;
    }

    m_Basket = FindEntityByName("Basket");
    if (!m_Basket)
        LOG_WARN("MyFirstGame: the scene has no Basket, nothing to play with");

    m_Crates.clear();
    m_Markers.clear();
    m_CaughtCrates.clear();
    m_Score = 0;
    m_Lives = k_StartingLives;
    m_GameOver = false;
    m_SpawnTimer = 0.0f;

    RefreshMarkers();
    LOG_INFO("MyFirstGame: catch the crates! Left/Right or A/D to move, Escape to quit");
}

void GameLayer::OnUpdate(Engine::Timestep ts)
{
    if (!m_SceneRuntime.GetScene())
        return;

    MoveBasket(ts);
    SpawnCrates(ts);

    m_SceneRuntime.OnUpdate(ts);

    // Après le pas de physique : c'est lui qui remplit les collisions et fait descendre
    // les caisses sous la ligne de perte.
    CollectCaughtCrates();
    CollectMissedCrates();

    Engine::Window &window = Engine::Application::Get().GetWindow();
    m_SceneRuntime.Render(window.GetWidth(), window.GetHeight());
    ++m_FrameCount;

    // Avant le swap de Window::OnUpdate : après, le back buffer ne contient plus ce
    // qu'on vient de dessiner.
    const bool lastFrame = m_Options.MaxFrames > 0 && m_FrameCount >= m_Options.MaxFrames;
    if (!m_Options.ScreenshotPath.empty() && lastFrame)
    {
        if (!Engine::ImageCapture::CaptureBackBuffer(m_Options.ScreenshotPath, window.GetWidth(), window.GetHeight()))
        {
            LOG_ERROR("MyFirstGame: could not write screenshot to {0}", m_Options.ScreenshotPath);
            m_ExitCode = 1;
        }
    }

    if (lastFrame)
    {
        if (m_Options.RequireCatch && m_Score == 0)
        {
            LOG_ERROR("MyFirstGame: no crate was caught in {0} frames", m_FrameCount);
            m_ExitCode = 1;
        }
        Engine::Application::Get().Close();
    }
}

void GameLayer::MoveBasket(Engine::Timestep ts)
{
    if (!m_Basket)
        return;

    Engine::PhysicsSystem &physics = m_SceneRuntime.GetPhysics();
    const auto &scene = m_SceneRuntime.GetScene();
    const float basketX = scene->GetWorldTransform(m_Basket).Position.x;

    float direction = 0.0f;
    if (m_Options.DemoMode)
    {
        // Vise la caisse la plus basse encore en jeu : c'est celle qu'il faut sauver.
        float targetX = basketX;
        float lowestY = k_SpawnHeight + 1.0f;
        for (Engine::UUID uuid : m_Crates)
        {
            Engine::Entity crate = scene->FindEntityByUUID(uuid);
            if (!crate)
                continue;

            const glm::vec3 position = scene->GetWorldTransform(crate).Position;
            if (position.y < lowestY)
            {
                lowestY = position.y;
                targetX = position.x;
            }
        }
        direction = std::clamp((targetX - basketX) * 4.0f, -1.0f, 1.0f);
    }
    else if (!m_GameOver)
    {
        if (Engine::Input::IsKeyPressed(Engine::Key::Left) || Engine::Input::IsKeyPressed(Engine::Key::A))
            direction -= 1.0f;
        if (Engine::Input::IsKeyPressed(Engine::Key::Right) || Engine::Input::IsKeyPressed(Engine::Key::D))
            direction += 1.0f;
    }

    physics.SetLinearVelocity(m_Basket, {direction * k_BasketSpeed, 0.0f});

    // Le panier est cinématique : rien ne l'arrête, c'est au jeu de le garder dans le
    // champ. La position lue date du pas précédent, un dépassement d'une frame est
    // rattrapé ici plutôt qu'anticipé.
    const float limit = k_PlayfieldHalfWidth - k_BasketHalfWidth;
    if (basketX < -limit || basketX > limit)
    {
        const float clampedX = std::clamp(basketX, -limit, limit);
        physics.SetPosition(m_Basket, {clampedX, scene->GetWorldTransform(m_Basket).Position.y});
        physics.SetLinearVelocity(m_Basket, {0.0f, 0.0f});
    }
}

void GameLayer::SpawnCrates(Engine::Timestep ts)
{
    if (m_GameOver)
        return;

    m_SpawnTimer += ts;
    if (m_SpawnTimer < k_SpawnInterval)
        return;
    m_SpawnTimer = 0.0f;

    const auto &scene = m_SceneRuntime.GetScene();
    std::uniform_real_distribution<float> spawnX(-1.3f, 1.3f);

    Engine::Entity crate = scene->CreateEntity(k_CrateTag);
    scene->SetParent(crate, scene->GetRootEntity());

    auto &transform = crate.GetComponent<Engine::TransformComponent>();
    transform.Position = {spawnX(m_Random), k_SpawnHeight, 0.0f};
    transform.Scale = {0.16f, 0.16f, 1.0f};

    crate.AddComponent<Engine::SpriteRendererComponent>(k_CrateColor);

    auto &rigidBody = crate.AddComponent<Engine::RigidBodyComponent>();
    rigidBody.Type = Engine::RigidBodyComponent::BodyType::Dynamic;

    auto &collider = crate.AddComponent<Engine::BoxColliderComponent>();
    collider.Size = {0.5f, 0.5f};
    collider.Friction = 0.4f;
    collider.Restitution = 0.0f;

    m_Crates.push_back(crate.GetUUID());
}

void GameLayer::CollectCaughtCrates()
{
    const auto &scene = m_SceneRuntime.GetScene();

    for (Engine::UUID uuid : m_CaughtCrates)
    {
        Engine::Entity crate = scene->FindEntityByUUID(uuid);
        if (!crate)
            continue; // déjà comptée : deux contacts dans le même pas

        scene->DestroyEntity(crate);
        m_Crates.erase(std::remove(m_Crates.begin(), m_Crates.end(), uuid), m_Crates.end());
        OnCrateCaught();
    }

    m_CaughtCrates.clear();
}

void GameLayer::CollectMissedCrates()
{
    const auto &scene = m_SceneRuntime.GetScene();

    for (auto it = m_Crates.begin(); it != m_Crates.end();)
    {
        Engine::Entity crate = scene->FindEntityByUUID(*it);
        if (crate && scene->GetWorldTransform(crate).Position.y > k_KillLine)
        {
            ++it;
            continue;
        }

        if (crate)
        {
            scene->DestroyEntity(crate);
            OnCrateMissed();
        }
        it = m_Crates.erase(it);
    }
}

void GameLayer::OnCrateCaught()
{
    ++m_Score;
    LOG_INFO("MyFirstGame: caught! score {0}", m_Score);

    if (m_Basket.HasComponent<Engine::AudioComponent>())
    {
        auto &audio = m_Basket.GetComponent<Engine::AudioComponent>();
        if (audio.Source)
            audio.Source->Play();
    }

    RefreshMarkers();
}

void GameLayer::OnCrateMissed()
{
    if (m_GameOver)
        return;

    --m_Lives;
    RefreshMarkers();

    if (m_Lives > 0)
    {
        LOG_INFO("MyFirstGame: missed! {0} live(s) left", m_Lives);
        return;
    }

    m_GameOver = true;
    LOG_INFO("MyFirstGame: game over — final score {0}. R to play again, Escape to quit", m_Score);
}

void GameLayer::RefreshMarkers()
{
    const auto &scene = m_SceneRuntime.GetScene();

    for (Engine::UUID uuid : m_Markers)
    {
        if (Engine::Entity marker = scene->FindEntityByUUID(uuid))
            scene->DestroyEntity(marker);
    }
    m_Markers.clear();

    // Score : une rangée qui se remplit depuis la gauche, repliée dès qu'elle atteint le
    // bord. Vies : depuis la droite, un carré de moins à chaque caisse ratée.
    for (int i = 0; i < m_Score; ++i)
    {
        const int row = i / k_MarkersPerRow;
        const int column = i % k_MarkersPerRow;
        CreateMarker({-1.45f + column * k_MarkerSpacing, k_MarkerRowY - row * k_MarkerSpacing}, k_ScoreColor);
    }

    for (int i = 0; i < m_Lives; ++i)
        CreateMarker({1.45f - i * k_MarkerSpacing, k_MarkerRowY}, k_LifeColor);
}

void GameLayer::CreateMarker(const glm::vec2 &position, const glm::vec4 &color)
{
    const auto &scene = m_SceneRuntime.GetScene();

    Engine::Entity marker = scene->CreateEntity(k_MarkerTag);
    scene->SetParent(marker, scene->GetRootEntity());

    auto &transform = marker.GetComponent<Engine::TransformComponent>();
    transform.Position = {position.x, position.y, 0.0f};
    transform.Scale = {k_MarkerSize, k_MarkerSize, 1.0f};

    marker.AddComponent<Engine::SpriteRendererComponent>(color);
    m_Markers.push_back(marker.GetUUID());
}

Engine::Entity GameLayer::FindEntityByName(const std::string &name)
{
    const auto &scene = m_SceneRuntime.GetScene();

    auto view = scene->GetAllEntitiesWith<Engine::TagComponent>();
    for (auto entityHandle : view)
    {
        Engine::Entity entity{entityHandle, scene.get()};
        if (entity.GetName() == name)
            return entity;
    }
    return {};
}

void GameLayer::OnEvent(Engine::Event &event)
{
    if (event.GetType() != Engine::EventType::KeyPressed)
        return;

    const int key = static_cast<Engine::KeyPressedEvent &>(event).GetKeyCode();
    if (key == Engine::Key::Escape)
    {
        Engine::Application::Get().Close();
        event.Handled = true;
    }
    else if (key == Engine::Key::R)
    {
        StartRound();
        event.Handled = true;
    }
}

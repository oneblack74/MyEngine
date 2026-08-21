#include "Assets/AssetManager.h"
#include "Core/Application.h"
#include "Core/Layer.h"
#include "Core/Log.h"
#include "Core/Input.h"
#include "Core/KeyCodes.h"
#include "Renderer/Renderer.h"
#include "Renderer/Renderer2D.h"
#include "Renderer/OrthographicCamera.h"
#include "Renderer/Texture.h"
#include "Scene/Scene.h"
#include "Scene/Entity.h"
#include "Scene/Components.h"
#include "Scene/RenderSystem.h"
#include "Scene/SceneManager.h"
#include <filesystem>

class GameLayer : public Engine::Layer
{
public:
    GameLayer() : Layer("GameLayer") {}

    void OnAttach() override
    {
        GAME_LOG_INFO("Gameplay", "GameLayer attached!");
        GAME_LOG_INFO("Gameplay", "Working dir: {0}", std::filesystem::current_path().string());

        Engine::Renderer2D::Init();

        m_TextureHandle = Engine::AssetManager::Import("assets/textures/axololt.jpg");
        m_Camera = std::make_shared<Engine::OrthographicCamera>(-1.6f, 1.6f, -0.9f, 0.9f);

        m_Scene = Engine::SceneManager::NewScene();

        auto redSquare = m_Scene->CreateEntity("RedSquare");
        auto &redTransform = redSquare.GetComponent<Engine::TransformComponent>();
        redTransform.Position = {-0.6f, 0.0f, 0.0f};
        redTransform.Scale = {0.5f, 0.5f, 1.0f};
        redSquare.AddComponent<Engine::SpriteRendererComponent>(glm::vec4(0.9f, 0.3f, 0.3f, 1.0f));

        auto greenSquare = m_Scene->CreateEntity("GreenSquare");
        auto &greenTransform = greenSquare.GetComponent<Engine::TransformComponent>();
        greenTransform.Position = {0.0f, -0.5f, 0.0f};
        greenTransform.Scale = {0.4f, 0.4f, 1.0f};
        greenTransform.Rotation = 33.0f; // pour vérifier que la rotation du TransformComponent est bien appliquée
        greenSquare.AddComponent<Engine::SpriteRendererComponent>(glm::vec4(0.3f, 0.8f, 0.4f, 1.0f));

        auto axolotl = m_Scene->CreateEntity("Axolotl");
        auto &axoTransform = axolotl.GetComponent<Engine::TransformComponent>();
        axoTransform.Position = {0.4f, 0.2f, 0.0f};
        axoTransform.Scale = {0.6f, 0.6f, 1.0f};
        auto &axoSprite = axolotl.AddComponent<Engine::SpriteRendererComponent>();
        axoSprite.Texture = m_TextureHandle;

        // Vérification manuelle : chaque entité doit avoir un UUID distinct
        GAME_LOG_INFO("Scène", "{0} UUID: {1}", redSquare.GetName(), (uint64_t)redSquare.GetUUID());
        GAME_LOG_INFO("Scène", "{0} UUID: {1}", greenSquare.GetName(), (uint64_t)greenSquare.GetUUID());
        GAME_LOG_INFO("Scène", "{0} UUID: {1}", axolotl.GetName(), (uint64_t)axolotl.GetUUID());

        // Test save/load/switch via SceneManager : on sauvegarde la scène active, on la
        // recharge (ce qui bascule le SceneManager dessus - "switch de scènes"), puis on
        // restaure explicitement m_Scene comme scène active pour le rendu.
        // Note : la texture de l'axolotl n'est pas encore sérialisée (pas d'asset system,
        // prévu en Phase 7), donc la scène rechargée l'afficherait en blanc si on la
        // rendait telle quelle — normal à ce stade.
        Engine::SceneManager::SaveActiveScene("demo_scene.json");

        auto reloadedScene = Engine::SceneManager::LoadScene("demo_scene.json");
        if (reloadedScene)
        {
            auto view = reloadedScene->GetAllEntitiesWith<Engine::IDComponent>();
            for (auto entityHandle : view)
            {
                Engine::Entity entity{entityHandle, reloadedScene.get()};
                GAME_LOG_INFO("Scène", "Reloaded: {0} UUID: {1}", entity.GetName(), (uint64_t)entity.GetUUID());
            }
        }

        Engine::SceneManager::SetActiveScene(m_Scene);
    }

    void OnDetach() override
    {
        Engine::Renderer2D::Shutdown();
    }

    void OnUpdate(Engine::Timestep ts) override
    {
        // Démo de l'Input System : déplace la caméra tant qu'une flèche est maintenue
        // (polling, à chaque frame — à comparer avec les KeyPressedEvent loggés en trace,
        // qui ne se déclenchent qu'une fois par appui).
        constexpr float cameraSpeed = 0.02f;
        glm::vec3 cameraPos = m_Camera->GetPosition();
        if (Engine::Input::IsKeyPressed(Engine::Key::Right))
            cameraPos.x += cameraSpeed;
        if (Engine::Input::IsKeyPressed(Engine::Key::Left))
            cameraPos.x -= cameraSpeed;
        if (Engine::Input::IsKeyPressed(Engine::Key::Up))
            cameraPos.y += cameraSpeed;
        if (Engine::Input::IsKeyPressed(Engine::Key::Down))
            cameraPos.y -= cameraSpeed;
        m_Camera->SetPosition(cameraPos);

        Engine::Renderer::SetClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        Engine::Renderer::Clear();

        Engine::RenderSystem::Render(*Engine::SceneManager::GetActiveScene(), *m_Camera);
    }

    void OnEvent(Engine::Event &event) override
    {
        GAME_LOG_TRACE("Gameplay", "GameLayer received: {0}", event.ToString());
    }

private:
    std::shared_ptr<Engine::OrthographicCamera> m_Camera;
    Engine::AssetHandle m_TextureHandle{Engine::k_InvalidAssetHandle};
    std::shared_ptr<Engine::Scene> m_Scene;
};

int main()
{
    Engine::Log::Init();
    LOG_INFO("Engine starting...");

    Engine::Application app;
    app.PushLayer(new GameLayer());
    app.Run();

    return 0;
}

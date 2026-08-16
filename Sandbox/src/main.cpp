#include "Core/Application.h"
#include "Core/Layer.h"
#include "Core/Log.h"
#include "Renderer/Renderer.h"
#include "Renderer/Renderer2D.h"
#include "Renderer/OrthographicCamera.h"
#include "Renderer/Texture.h"
#include "Scene/Scene.h"
#include "Scene/Entity.h"
#include "Scene/Components.h"
#include "Scene/RenderSystem.h"
#include "Scene/SceneSerializer.h"
#include <filesystem>

class GameLayer : public Engine::Layer
{
public:
    GameLayer() : Layer("GameLayer") {}

    void OnAttach() override
    {
        LOG_INFO("GameLayer attached!");
        LOG_INFO("Working dir: {0}", std::filesystem::current_path().string());

        Engine::Renderer2D::Init();

        m_Texture = std::make_shared<Engine::Texture2D>("assets/textures/axololt.jpg");
        m_Camera = std::make_shared<Engine::OrthographicCamera>(-1.6f, 1.6f, -0.9f, 0.9f);

        m_Scene = std::make_shared<Engine::Scene>();

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
        axoSprite.Texture = m_Texture;

        // Vérification manuelle : chaque entité doit avoir un UUID distinct
        LOG_INFO("{0} UUID: {1}", redSquare.GetName(), (uint64_t)redSquare.GetUUID());
        LOG_INFO("{0} UUID: {1}", greenSquare.GetName(), (uint64_t)greenSquare.GetUUID());
        LOG_INFO("{0} UUID: {1}", axolotl.GetName(), (uint64_t)axolotl.GetUUID());

        // Test round-trip de la sérialisation : on écrit la scène sur disque, puis on la
        // recharge dans une Scene séparée (pas utilisée pour le rendu) pour vérifier que
        // les entités et leurs UUID sont bien préservés après un cycle save/load.
        Engine::SceneSerializer serializer(m_Scene);
        serializer.Serialize("demo_scene.json");

        auto reloadedScene = std::make_shared<Engine::Scene>();
        Engine::SceneSerializer loader(reloadedScene);
        if (loader.Deserialize("demo_scene.json"))
        {
            auto view = reloadedScene->GetAllEntitiesWith<Engine::IDComponent>();
            for (auto entityHandle : view)
            {
                Engine::Entity entity{entityHandle, reloadedScene.get()};
                LOG_INFO("Reloaded: {0} UUID: {1}", entity.GetName(), (uint64_t)entity.GetUUID());
            }
        }
    }

    void OnDetach() override
    {
        Engine::Renderer2D::Shutdown();
    }

    void OnUpdate() override
    {
        Engine::Renderer::SetClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        Engine::Renderer::Clear();

        Engine::RenderSystem::Render(*m_Scene, *m_Camera);
    }

    void OnEvent(Engine::Event &event) override
    {
        LOG_TRACE("GameLayer received: {0}", event.ToString());
    }

private:
    std::shared_ptr<Engine::OrthographicCamera> m_Camera;
    std::shared_ptr<Engine::Texture2D> m_Texture;
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

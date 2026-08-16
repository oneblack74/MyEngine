#include "Core/Application.h"
#include "Core/Layer.h"
#include "Core/Log.h"
#include "Renderer/Renderer.h"
#include "Renderer/Renderer2D.h"
#include "Renderer/SubTexture2D.h"
#include "Renderer/OrthographicCamera.h"
#include "Renderer/Texture.h"
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

        // Démo SubTexture2D : on traite l'image (1400x1400) comme une grille 2x2
        // et on n'affiche que la cellule (0,0), pour vérifier le découpage UV.
        m_SubTexture = Engine::SubTexture2D::CreateFromCoords(m_Texture, {0.0f, 0.0f}, {700.0f, 700.0f});

        m_Camera = std::make_shared<Engine::OrthographicCamera>(-1.6f, 1.6f, -0.9f, 0.9f);
    }

    void OnDetach() override
    {
        Engine::Renderer2D::Shutdown();
    }

    void OnUpdate() override
    {
        Engine::Renderer::SetClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        Engine::Renderer::Clear();

        Engine::Renderer2D::BeginScene(*m_Camera);
        Engine::Renderer2D::DrawQuad({-0.6f, 0.0f}, {0.5f, 0.5f}, glm::vec4(0.9f, 0.3f, 0.3f, 1.0f));
        Engine::Renderer2D::DrawQuad({0.0f, -0.5f}, {0.4f, 0.4f}, glm::vec4(0.3f, 0.8f, 0.4f, 1.0f));
        Engine::Renderer2D::DrawQuad({0.4f, 0.2f}, {0.6f, 0.6f}, m_Texture);
        Engine::Renderer2D::DrawQuad({-1.0f, -0.5f}, {0.5f, 0.5f}, m_SubTexture);
        Engine::Renderer2D::EndScene();
    }

    void OnEvent(Engine::Event &event) override
    {
        LOG_TRACE("GameLayer received: {0}", event.ToString());
    }

private:
    std::shared_ptr<Engine::OrthographicCamera> m_Camera;
    std::shared_ptr<Engine::Texture2D> m_Texture;
    std::shared_ptr<Engine::SubTexture2D> m_SubTexture;
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

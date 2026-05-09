#include "Core/Application.h"
#include "Core/Layer.h"
#include "Core/Log.h"
#include "Renderer/Renderer.h"
#include "Renderer/Buffer.h"
#include "Renderer/VertexArray.h"
#include "Renderer/Shader.h"
#include "Renderer/OrthographicCamera.h"
#include <iostream>

class GameLayer : public Engine::Layer
{
public:
    GameLayer() : Layer("GameLayer") {}

    void OnAttach() override
    {
        std::cout << "GameLayer attached!" << std::endl;

        // Triangle
        float vertices[] = {
            -0.5f, -0.5f, 0.0f,
            0.5f, -0.5f, 0.0f,
            0.0f, 0.5f, 0.0f};
        uint32_t indices[] = {0, 1, 2};

        m_VertexArray = std::make_shared<Engine::VertexArray>();

        auto vb = std::make_shared<Engine::VertexBuffer>(vertices, sizeof(vertices));
        vb->SetLayout({{Engine::ShaderDataType::Float3, "a_Position"}});
        m_VertexArray->AddVertexBuffer(vb);

        auto ib = std::make_shared<Engine::IndexBuffer>(indices, 3);
        m_VertexArray->SetIndexBuffer(ib);

        std::string vertexSrc = R"(
            #version 330 core
            layout(location = 0) in vec3 a_Position;
            uniform mat4 u_ViewProjection;
            void main()
            {
                gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
            }
        )";

        std::string fragmentSrc = R"(
            #version 330 core
            out vec4 color;
            void main()
            {
                color = vec4(0.2, 0.8, 0.3, 1.0);
            }
        )";

        m_Shader = std::make_shared<Engine::Shader>(vertexSrc, fragmentSrc);

        m_Camera = std::make_shared<Engine::OrthographicCamera>(-1.6f, 1.6f, -0.9f, 0.9f);
        m_Camera->SetPosition({0.3f, 0.3f, 0.0f});
    }

    void OnUpdate() override
    {
        Engine::Renderer::SetClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        Engine::Renderer::Clear();

        m_Shader->Bind();
        m_Shader->SetMat4("u_ViewProjection", m_Camera->GetViewProjectionMatrix());
        m_VertexArray->Bind();
        glDrawElements(GL_TRIANGLES, m_VertexArray->GetIndexBuffer()->GetCount(), GL_UNSIGNED_INT, nullptr);
    }

    void OnEvent(Engine::Event &event) override
    {
        std::cout << "GameLayer received: " << event.ToString() << std::endl;
    }

private:
    std::shared_ptr<Engine::VertexArray> m_VertexArray;
    std::shared_ptr<Engine::Shader> m_Shader;
    std::shared_ptr<Engine::OrthographicCamera> m_Camera;
};

int main()
{
    Engine::Log::Init();
    Engine::LOG_INFO("Engine starting...");

    MyEngine::Application app;
    app.PushLayer(new GameLayer());
    app.Run();

    return 0;
}

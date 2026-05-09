#include "Core/Application.h"
#include "Core/Layer.h"
#include "Core/Log.h"
#include <iostream>

class GameLayer : public Engine::Layer
{
public:
    GameLayer() : Layer("GameLayer") {}

    void OnAttach() override
    {
        std::cout << "GameLayer attached!" << std::endl;
    }

    void OnUpdate() override
    {
        // appelé chaque frame
    }

    void OnEvent(Engine::Event &event) override
    {
        std::cout << "GameLayer received: " << event.ToString() << std::endl;
    }
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

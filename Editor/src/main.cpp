#include "EditorLayer.h"
#include <Core/Application.h>
#include <Core/Log.h>

int main()
{
    Engine::Log::Init();
    LOG_INFO("Editor starting...");

    Engine::Application app;
    app.PushLayer(new EditorLayer());
    app.Run();

    return 0;
}

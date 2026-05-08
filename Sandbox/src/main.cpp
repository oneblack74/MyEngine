#include "Core/Window.h"
#include <iostream>

int main()
{
    Engine::Window window({});

    window.SetEventCallback([](Engine::Event &event)
                            { std::cout << event.GetName() << std::endl; });

    while (!window.ShouldClose())
    {
        window.OnUpdate();
    }

    return 0;
}

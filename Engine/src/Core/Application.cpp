#include "Core/Application.h"
#include <iostream>

namespace MyEngine
{
    Application::Application()
    {
        std::cout << "Engine initialized\n";
    }

    Application::~Application()
    {
        std::cout << "Engine shutdown\n";
    }

    void Application::Run()
    {
        std::cout << "Engine running\n";
    }
}

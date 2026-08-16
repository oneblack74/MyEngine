#pragma once

// Affiche les messages loggés via spdlog (Engine::Log) directement dans l'éditeur.
class ConsolePanel
{
public:
    void OnImGuiRender();
};

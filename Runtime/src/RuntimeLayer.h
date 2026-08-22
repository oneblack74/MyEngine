#pragma once
#include "RuntimeOptions.h"
#include <Core/Layer.h>
#include <Scene/SceneRuntime.h>

// Le jeu tel que le joueur le reçoit : une scène chargée depuis son fichier, jouée
// immédiatement (pas d'état « édition », pas de bouton Play), rendue plein écran par la
// caméra de la scène. Toute la mécanique est dans Engine::SceneRuntime ; ce layer ne
// fait que la brancher sur la fenêtre et sur les options de la ligne de commande.
class RuntimeLayer : public Engine::Layer
{
public:
    explicit RuntimeLayer(const RuntimeOptions &options);

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(Engine::Timestep ts) override;
    void OnEvent(Engine::Event &event) override;

    // 0 si tout s'est bien passé, 1 si la scène n'a pas pu être chargée ou la capture
    // écrite. Lu par main() après la boucle, pour que ctest voie l'échec.
    int GetExitCode() const { return m_ExitCode; }

private:
    RuntimeOptions m_Options;
    Engine::SceneRuntime m_SceneRuntime;

    int m_FrameCount = 0;
    int m_ExitCode = 0;
};

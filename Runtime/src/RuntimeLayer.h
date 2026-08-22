#pragma once
#include "RuntimeOptions.h"
#include <Core/Layer.h>
#include <Scene/PhysicsSystem.h>
#include <Scene/Scene.h>
#include <memory>

// Le jeu tel que le joueur le reçoit : une scène chargée depuis son fichier, jouée
// immédiatement (pas d'état "édition", pas de bouton Play), rendue plein écran par la
// caméra de la scène. C'est le pendant sans éditeur de EditorLayer.
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
    void RenderFrame();

    RuntimeOptions m_Options;
    std::shared_ptr<Engine::Scene> m_Scene;
    Engine::PhysicsSystem m_PhysicsSystem;

    int m_FrameCount = 0;
    bool m_WarnedNoCamera = false;
    int m_ExitCode = 0;
};

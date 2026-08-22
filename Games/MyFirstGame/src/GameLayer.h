#pragma once
#include "GameOptions.h"
#include <Core/Layer.h>
#include <Core/UUID.h>
#include <Scene/Entity.h>
#include <Scene/SceneRuntime.h>
#include <glm/glm.hpp>
#include <random>
#include <vector>

// Attrape-caisses : des caisses tombent du haut de l'écran, le panier se déplace de
// gauche à droite pour les rattraper. Une caisse ratée coûte une vie, trois vies
// perdues et la partie est finie (R pour rejouer).
//
// Toute la logique est ici, en C++ : tant qu'il n'y a pas de scripting (Phase 9), un
// jeu est un exécutable qui pose son propre layer par-dessus Engine::SceneRuntime,
// là où le Runtime générique se contente de jouer une scène telle quelle.
class GameLayer : public Engine::Layer
{
public:
    explicit GameLayer(const GameOptions &options);

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(Engine::Timestep ts) override;
    void OnEvent(Engine::Event &event) override;

    int GetExitCode() const { return m_ExitCode; }

private:
    void StartRound();
    void MoveBasket(Engine::Timestep ts);
    void SpawnCrates(Engine::Timestep ts);
    // Encaisse ce que le pas de physique vient de produire : caisses attrapées (par les
    // events de collision) et caisses tombées trop bas.
    void CollectCaughtCrates();
    void CollectMissedCrates();
    void OnCrateCaught();
    void OnCrateMissed();
    // Score et vies se lisent sur des rangées de petits carrés, faute de rendu de texte
    // dans le moteur : rangée reconstruite à chaque changement.
    void RefreshMarkers();
    void CreateMarker(const glm::vec2 &position, const glm::vec4 &color);

    Engine::Entity FindEntityByName(const std::string &name);

    GameOptions m_Options;
    Engine::SceneRuntime m_SceneRuntime;

    Engine::Entity m_Basket;
    std::vector<Engine::UUID> m_Crates;
    std::vector<Engine::UUID> m_Markers;
    // Remplie par le callback de collision pendant le pas de physique, vidée juste
    // après : détruire une entité au milieu des events laisserait les suivants pointer
    // sur un handle mort.
    std::vector<Engine::UUID> m_CaughtCrates;

    int m_Score = 0;
    int m_Lives = 0;
    bool m_GameOver = false;
    float m_SpawnTimer = 0.0f;

    // Graine fixe : deux parties lancées dans les mêmes conditions tombent sur les
    // mêmes caisses, ce qui rend un test reproductible.
    std::mt19937 m_Random{1234};

    int m_FrameCount = 0;
    int m_ExitCode = 0;
};

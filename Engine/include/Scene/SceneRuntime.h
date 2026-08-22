#pragma once
#include "Scene/Entity.h"
#include "Scene/PhysicsSystem.h"
#include "Scene/Scene.h"
#include <glm/glm.hpp>
#include <memory>
#include <string>

namespace Engine
{
    // Fait tourner une scène « en jeu » : démarre la physique et l'audio, les fait
    // avancer à chaque frame, et dessine la scène par sa caméra Primary. C'est le
    // dénominateur commun entre le player standalone et un jeu qui ajoute sa logique
    // par-dessus. L'éditeur garde son propre pilotage (Play/Pause/Stop sur une copie
    // jetable, rendu dans le Framebuffer d'un panel) : sa scène n'est pas seulement
    // jouée, elle est aussi éditée.
    class SceneRuntime
    {
    public:
        ~SceneRuntime();

        // Charge la scène depuis son fichier et démarre la partie. Renvoie false si le
        // fichier ne se charge pas — auquel cas rien n'a démarré.
        bool Start(const std::string &scenePath);
        void Start(const std::shared_ptr<Scene> &scene);

        // Sans effet si aucune partie n'est en cours : appelable deux fois de suite,
        // et appelé de toute façon par le destructeur.
        void Stop();

        // Physique et audio. Ne dessine rien.
        void OnUpdate(float timestep);

        // Dessine la scène dans la cible de rendu courante, à la taille donnée (celle
        // de la fenêtre, en général).
        void Render(uint32_t width, uint32_t height);

        // Première caméra marquée Primary, entité nulle s'il n'y en a pas.
        Entity GetPrimaryCamera();

        const std::shared_ptr<Scene> &GetScene() const { return m_Scene; }
        PhysicsSystem &GetPhysics() { return m_PhysicsSystem; }

        glm::vec4 ClearColor{0.1f, 0.1f, 0.1f, 1.0f};

    private:
        std::shared_ptr<Scene> m_Scene;
        PhysicsSystem m_PhysicsSystem;

        // Une scène sans caméra ne dessine rien : le dire une fois, pas à chaque frame.
        bool m_WarnedNoCamera = false;
    };
}

#pragma once
#include "Scene/Scene.h"

namespace Engine
{
    // Fait vivre les AudioComponent pendant le Play : charge les sons au démarrage,
    // joue ceux marqués PlayOnStart, et les relâche à l'arrêt.
    class AudioSystem
    {
    public:
        static void OnRuntimeStart(Scene &scene);

        // Repousse les réglages des components vers les sons à chaque frame, pour que
        // les éditer dans l'Inspecteur pendant le Play s'entende tout de suite — même
        // raison d'être que PhysicsSystem::SyncComponentsToBox2D.
        static void OnUpdate(Scene &scene);

        static void OnRuntimeStop(Scene &scene);
    };
}
